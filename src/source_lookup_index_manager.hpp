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
 * Provides interfaces for managing source lookup storage.
 * Specifically, manage the association between an index
 * and its associated repository_name, filename pair.
 */

#ifndef SOURCE_LOOKUP_INDEX_MANAGER_HPP
#define SOURCE_LOOKUP_INDEX_MANAGER_HPP
#include "dfxml/src/dfxml_writer.h"
#include "bi_data_types.hpp"
#include <boost/btree/support/string_view.hpp>
#include "bi_store.hpp"
#include <string>

// a lock is required to protect btree
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

/**
 * Provides interfaces for managing source lookup storage.
 * Source lookup storage is implemented using three bidirectional maps:
 *
 *   source_lookup_store:
 *       key=source_lookup_index,
 *       value=pair(repository_name_index, filename_index)
 *   repository_name_lookup_store:
 *       key=repository_name_index
 *       value=string_view
 *   filename_lookup_store:
 *       key=filename_index
 *       value=string_view
 */
class source_lookup_index_manager_t {
  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode_type;

  typedef bi_store_t<bi_data_64_pair_t> source_lookup_store_t;
  typedef bi_store_t<bi_data_64_sv_t>   repository_name_lookup_store_t;
  typedef bi_store_t<bi_data_64_sv_t>   filename_lookup_store_t;

  source_lookup_store_t          source_lookup_store;
  repository_name_lookup_store_t repository_name_lookup_store;
  filename_lookup_store_t        filename_lookup_store;
#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // helper
  std::string to_string(const boost::string_view& sv) const {
    std::string s = "";
    boost::string_view::const_iterator it = sv.begin();
    while (it != sv.end()) {
      s.push_back(*it);
      ++it;
    }
    return s;
  }

  // disallow these
  source_lookup_index_manager_t(const source_lookup_index_manager_t&);
  source_lookup_index_manager_t& operator=(const source_lookup_index_manager_t&);

  public:
  typedef boost::btree::btree_index_set<bi_data_64_pair_t>::iterator source_lookup_index_iterator_t;

  source_lookup_index_manager_t (const std::string p_hashdb_dir,
                           file_mode_type_t p_file_mode_type) :
       hashdb_dir(p_hashdb_dir), file_mode_type(p_file_mode_type),
       source_lookup_store(
            hashdb_dir + "/source_lookup_store", file_mode_type),
       repository_name_lookup_store(
            hashdb_dir + "/source_repository_name_store", file_mode_type),
       filename_lookup_store(
            hashdb_dir + "/source_filename_store", file_mode_type),
            M() {
    MUTEX_INIT(&M);
  }

  /**
   * Insert and return true and the next source lookup index
   * else return false and existing source_lookup_index if already there.
   */
  std::pair<bool, uint64_t> insert(const std::string& repository_name,
                                   const std::string& filename) {

    MUTEX_LOCK(&M);

    // get or make repository name index from repository name
    boost::string_view repository_name_sv(repository_name);
    uint64_t repository_name_index;
    bool status1 = repository_name_lookup_store.get_key(
                                repository_name_sv, repository_name_index);
    if (status1 == false) {
      // add new repository name element, returning new index
      repository_name_index = repository_name_lookup_store.insert_value(repository_name_sv);
    }

    // get or make filename index from filename
    boost::string_view filename_sv(filename);
    uint64_t filename_index;
    bool status2 = filename_lookup_store.get_key(filename_sv, filename_index);
    if (status2 == false) {
      // add new repository name using its new key
      filename_index = filename_lookup_store.insert_value(filename_sv);
    }

    // define the index_pair value for the source lookup store
    std::pair<uint64_t, uint64_t> index_pair(repository_name_index, filename_index);

    // look for existing key
    uint64_t source_lookup_index;
    bool status3 = source_lookup_store.get_key(
                         index_pair, source_lookup_index);

    // either use existing key or add element and use new key
    if (status3 == false) {
      // insert the new element
      source_lookup_index = source_lookup_store.insert_value(index_pair);
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(true, source_lookup_index);
    } else {
      // just offer the index from the existing element
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(false, source_lookup_index);
    }
  }

  /**
   * Find the source lookup index from the repository name and the filename,
   * returning pair(false, 0) if not present.
   */
  std::pair<bool, uint64_t> find(const std::string& repository_name,
                            const std::string& filename) const {

    // get repository name index from repository name
    boost::string_view repository_name_sv(repository_name);
    uint64_t repository_name_index;
    MUTEX_LOCK(&M);
    bool status1 = repository_name_lookup_store.get_key(
                                repository_name_sv, repository_name_index);
    if (status1 == false) {
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(false, 0);
    }

    // get filename index from filename
    boost::string_view filename_sv(filename);
    uint64_t filename_index;
    bool status2 = filename_lookup_store.get_key(filename_sv, filename_index);
    if (status2 == false) {
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(false, 0);
    }

    // get source lookup index from looked up index values
    std::pair<uint64_t, uint64_t> index_pair(repository_name_index, filename_index);
    uint64_t source_lookup_index;
    bool status3 = source_lookup_store.get_key(index_pair, source_lookup_index);
    if (status3 == false) {
      MUTEX_UNLOCK(&M);
      return std::pair<bool, uint64_t>(false, 0);
    }

    MUTEX_UNLOCK(&M);
    return std::pair<bool, uint64_t>(true, source_lookup_index);
  }

  /**
   * Find the repository name and filename from the source lookup index.
   */
  
  std::pair<bool, std::pair<std::string, std::string> >
                          find(uint64_t source_lookup_index) const {

    typedef std::pair<std::string, std::string> string_pair_t;

    MUTEX_LOCK(&M);

    // get the lookup pair from the index
    std::pair<uint64_t, uint64_t> lookup_pair;
    bool status1 = source_lookup_store.get_value(
                                     source_lookup_index, lookup_pair);
    if (status1 == false) {
      MUTEX_UNLOCK(&M);
      return std::pair<bool, string_pair_t>(false, string_pair_t("",""));
    }

    // get repository name from repository name index
    boost::string_view repository_name_sv;
    bool status2 = repository_name_lookup_store.get_value(
                                    lookup_pair.first, repository_name_sv);
    if (status2 != true) {
      MUTEX_UNLOCK(&M);
      return std::pair<bool, string_pair_t>(false, string_pair_t("",""));
    }

    // get filename from filename index
    boost::string_view filename_sv;
    bool status3 = filename_lookup_store.get_value(
                                    lookup_pair.second, filename_sv);
    if (status3 != true) {
      MUTEX_UNLOCK(&M);
      return std::pair<bool, string_pair_t>(false, string_pair_t("",""));
    }

    std::pair<std::string, std::string> string_pair(to_string(repository_name_sv),
                                               to_string(filename_sv));
    MUTEX_UNLOCK(&M);
    return std::pair<bool, string_pair_t>(true, string_pair);
  }

  // begin
  source_lookup_index_iterator_t begin() const {
    return source_lookup_store.index_by_key_begin();
  }

  // end
  source_lookup_index_iterator_t end() const {
    return source_lookup_store.index_by_key_end();
  }

  // sizes
  size_t source_lookup_store_size() const {
    return source_lookup_store.size();
  }
  size_t repository_name_lookup_store_size() const {
    return repository_name_lookup_store.size();
  }
  size_t filename_lookup_store_size() const {
    return filename_lookup_store.size();
  }
};

#endif

