// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Provides services for accessing the hash store, including tracking changes.
 */

#ifndef HASHDB_MANAGER_HPP
#define HASHDB_MANAGER_HPP
#include "file_modes.h"
#include "hashdb_settings.hpp"
#include "source_lookup_index_manager.hpp"
#include "hashdb_changes.hpp"
#include "hashdb_element.hpp"
#include "bloom_filter_manager.hpp"
#include "source_lookup_encoding.hpp"
#include "source_metadata.hpp"
#include "source_metadata_element.hpp"
#include "source_metadata_manager.hpp"
#include "hash_t_selector.h"
#include "globals.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include "lmdb_resources.h"
#include "lmdb_hash_store_manager.hpp"

// hash store and hash iterator
typedef lmdb_hash_store_manager_t hash_store_t;
typedef lmdb_hash_store_iterator_t hash_store_key_iterator_t;
typedef std::pair<hash_store_key_iterator_t, hash_store_key_iterator_t>
                                           hash_store_key_iterator_range_t;

// fetch key and value from key_iterator
inline hash_t key(const hash_store_key_iterator_t &it) {
  return it->first;
}
inline uint64_t value(const hash_store_key_iterator_t &it) {
  return it->second;
}

class hashdb_manager_t {

  public:

  const std::string hashdb_dir;
  const file_mode_type_t file_mode;
  const hashdb_settings_t settings;
  hashdb_changes_t changes;

  private:
  // hash_store_key
  hash_store_t hash_store_key;

  // bloom filter manager
  bloom_filter_manager_t bloom_filter_manager;

  // source lookup support
  source_lookup_index_manager_t source_lookup_index_manager;

  // source metadata lookup support
  source_metadata_manager_t source_metadata_manager;

  // do not allow copy or assignment
  hashdb_manager_t(const hashdb_manager_t&);
  hashdb_manager_t& operator=(const hashdb_manager_t&);

  public:
  hashdb_manager_t(const std::string& p_hashdb_dir,
                   file_mode_type_t p_file_mode) :
                hashdb_dir(p_hashdb_dir),
                file_mode(p_file_mode),
                settings(hashdb_settings_store_t::read_settings(hashdb_dir)),
                changes(),
                hash_store_key(hashdb_dir, file_mode),
                bloom_filter_manager(hashdb_dir, file_mode,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions),
                source_lookup_index_manager(hashdb_dir, file_mode),
                source_metadata_manager(hashdb_dir, file_mode) {
  }

  /**
   * Return a hashdb_element given a hash_store_key_iterator.
   */
  hashdb_element_t hashdb_element(const hash_store_key_iterator_t& it) const {
    uint64_t source_lookup_index = source_lookup_encoding::get_source_lookup_index(value(it));
    std::pair<bool, std::pair<std::string, std::string> > source_pair =
                     source_lookup_index_manager.find(source_lookup_index);
    if (source_pair.first == false) {
      std::cerr << "Invalid source lookup index value " << source_lookup_index
                << " encountered during element lookup.\nDatabase "
                << hashdb_dir << " may be corrupt.  Aborting.\n";
      exit(1);
    }
    return hashdb_element_t(
                    key(it),
                    settings.hash_block_size,
                    source_pair.second.first,
                    source_pair.second.second,
                    source_lookup_encoding::get_file_offset(value(it)));
  }

  /**
   * Return source lookup index given hash_store_key_iterator.
   */
  uint64_t source_id(const hash_store_key_iterator_t& it) const {
    return source_lookup_encoding::get_source_lookup_index(value(it));
  }

  /**
   * Return file offset given hash_store_key_iterator.
   */
  uint64_t file_offset(const hash_store_key_iterator_t& it) const {
    return source_lookup_encoding::get_file_offset(value(it));
  }

  // insert
  void insert(const hashdb_element_t& element) {

    // validate block size
    if (settings.hash_block_size != 0 &&
        (element.hash_block_size != settings.hash_block_size)) {
      ++changes.hashes_not_inserted_mismatched_hash_block_size;
      return;
    }

    // validate the byte alignment, see configure.ac for HASHDB_BYTE_ALIGNMENT
    if (element.file_offset % HASHDB_BYTE_ALIGNMENT != 0) {
      ++changes.hashes_not_inserted_invalid_byte_alignment;
      return;
    }

    // checks passed, insert or have reason not to insert

    // acquire existing or new source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.insert(element.repository_name,
                                            element.filename);
    uint64_t source_lookup_index = lookup_pair.second;

    // compose the source lookup encoding
    uint64_t encoding = source_lookup_encoding::get_source_lookup_encoding(
                       source_lookup_index,
                       element.file_offset);

    // if the key may exist then check against duplicates and max count
    if (bloom_filter_manager.is_positive(element.key)) {

      // disregard if key, value exists
      if (hash_store_key.find(element.key, encoding)) {
        // this exact element already exists
        ++changes.hashes_not_inserted_duplicate_element;
        return;
      }

      // disregard if above max duplicates
      if (settings.maximum_hash_duplicates > 0) {
        size_t count = hash_store_key.count(element.key);
        if (count >= settings.maximum_hash_duplicates) {
          // at maximum for this hash
          ++changes.hashes_not_inserted_exceeds_max_duplicates;
          return;
        }
      }
    }

    // add the element since all the checks passed
    hash_store_key.emplace(element.key, encoding);
    ++changes.hashes_inserted;

    // add hash to bloom filter, too, even if already there
    bloom_filter_manager.add_hash_value(element.key);
  }

  // insert source
  uint64_t insert_source(const std::string& repository_name,
                     const std::string& filename) {

    // acquire existing or new source lookup index
    std::pair<bool, uint64_t> lookup_pair =
              source_lookup_index_manager.insert(repository_name, filename);
    return lookup_pair.second;
  }

  // insert source metadata
  void insert_source_metadata(uint64_t source_lookup_index,
                              uint64_t filesize,
                              hash_t hashdigest) {

    // insert the metadata into the source metadata store
    bool status = source_metadata_manager.insert(
                                   source_lookup_index, filesize, hashdigest);

    // log change
    if (status == true) {
      ++changes.source_metadata_inserted;
    } else {
      ++changes.source_metadata_not_inserted_already_present;
    }
  }

  // remove
  void remove(const hashdb_element_t& element) {

    // validate block size
    if (settings.hash_block_size != 0 &&
        (element.hash_block_size != settings.hash_block_size)) {
      ++changes.hashes_not_removed_mismatched_hash_block_size;
      return;
    }

    // validate the byte alignment, see configure.ac for HASHDB_BYTE_ALIGNMENT
    if (element.file_offset % HASHDB_BYTE_ALIGNMENT != 0) {
      ++changes.hashes_not_removed_invalid_byte_alignment;
      return;
    }

    // find source lookup index
    std::pair<bool, uint64_t> lookup_pair =
         source_lookup_index_manager.find(element.repository_name,
                                          element.filename);
    if (lookup_pair.first == false) {
      ++changes.hashes_not_removed_no_element; // because there was no source
      return;
    }

    uint64_t source_lookup_index = lookup_pair.second;

    // compose the source lookup encoding
    uint64_t encoding = source_lookup_encoding::get_source_lookup_encoding(
                       source_lookup_index,
                       element.file_offset);

    // find and remove the distinct identified element
    bool did_erase = hash_store_key.erase(element.key, encoding);
    if (did_erase) {
      ++changes.hashes_removed;
      return;
    }

    // the key with the source lookup encoding was not found
    ++changes.hashes_not_removed_no_element;
    return;
  }

  // remove hash
  void remove_hash(const hash_t& hash) {

    // erase elements of hash
    uint32_t count = hash_store_key.erase(hash);
    
    if (count == 0) {
      // no hash
      ++changes.hashes_not_removed_no_hash;
    } else {
      changes.hashes_removed += count;
    }
  }

  /**
   * Find returning a hash_store_key iterator pair.
   */

  hash_store_key_iterator_range_t find(const hash_t& key) const {
    // if key not in bloom filter then return empty range
    if (!bloom_filter_manager.is_positive(key)) {
      return hash_store_key_iterator_range_t(
                                 hash_store_key.end(), hash_store_key.end());
    }

    // return range from hash_store_key
    return hash_store_key.equal_range(key);
  }

  // find_count
  uint32_t find_count(const hash_t& key) const {
    // if key not in bloom filter then clearly count=0
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present in bloom filter
      return 0;
    } else {
      // return count from hash_store_key
      return hash_store_key.count(key);
    }
  }

  /**
   * Return true and source lookup index else false and 0.
   */
  std::pair<bool, uint64_t> find_source_id(
                                     const std::string& repository_name,
                                     const std::string& filename) const {
    return source_lookup_index_manager.find(repository_name, filename);
  }

  /**
   * Find the source pair of repository name and filenam strings
   * from the source lookup index.
   */
  std::pair<bool, std::pair<std::string, std::string> > find_source(
                                      uint64_t source_lookup_index) const {
    return source_lookup_index_manager.find(source_lookup_index);
  }

  /**
   * Obtain source metadata given the source lookup index.
   */
  std::pair<bool, source_metadata_t> find_source_metadata(
                                     uint64_t source_lookup_index) const {
    return source_metadata_manager.find(source_lookup_index);
  }

  // begin
  hash_store_key_iterator_t begin_key() const {
    return hash_store_key.begin();
  }

  // end
  hash_store_key_iterator_t end_key() const {
    return hash_store_key.end();
  }

  // begin source lookup index iterator
  source_lookup_index_manager_t::source_lookup_index_iterator_t begin_source_lookup_index() const {
    return source_lookup_index_manager.begin();
  }

  // end source lookup index iterator
  source_lookup_index_manager_t::source_lookup_index_iterator_t end_source_lookup_index() const {
    return source_lookup_index_manager.end();
  }

  // hash_store_key size
  size_t map_size() const {
    return hash_store_key.size();
  }

  // source lookup store size
  size_t source_lookup_store_size() const {
    return source_lookup_index_manager.source_lookup_store_size();
  }

  // repository name lookup store size
  size_t repository_name_lookup_store_size() const {
    return source_lookup_index_manager.repository_name_lookup_store_size();
  }

  // filename lookup store size
  size_t filename_lookup_store_size() const {
    return source_lookup_index_manager.filename_lookup_store_size();
  }

  // source metadata lookup store size
  size_t source_metadata_lookup_store_size() const {
    return source_metadata_manager.size();
  }
};

#endif

