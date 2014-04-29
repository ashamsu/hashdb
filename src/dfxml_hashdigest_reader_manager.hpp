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
 * Unfortunately, the existing hashdigest reader output is hard to consume.
 * To get by, this manager reads all entries into a vector of hashdb_element_t
 * and allows the consumer to use an iterator on the vector.
 * Not efficient, but the interface presented is clean.
 */

#ifndef DFXML_HASHDIGEST_READER_MANAGER_HPP
#define DFXML_HASHDIGEST_READER_MANAGER_HPP
#include "hashdb_element.hpp"
#include "dfxml_hashdigest_reader.hpp"

// Standard includes
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
//#include <algorithm>
#include <vector>

template<typename T>
class dfxml_hashdigest_reader_manager_t {

  private:
  typedef std::vector<hashdb_element_t<T> > hashdb_elements_t;
  const std::string dfxml_filename;
  const std::string default_repository_name;
  hashdb_elements_t* elements;

  // do not allow copy or assignment
  dfxml_hashdigest_reader_manager_t(const dfxml_hashdigest_reader_manager_t&);
  dfxml_hashdigest_reader_manager_t& operator=(const dfxml_hashdigest_reader_manager_t&);

  // internal consumer for vector
  // during initialization, this reader uses this consumer with
  // dfxml_hashdigest_reader's static do_read() function.
  class reader_consumer_t {
    hashdb_elements_t* elements;

    public:
    reader_consumer_t(hashdb_elements_t* p_elements) :
      elements(p_elements) {
    }

    void consume(const hashdb_element_t<T>& hashdb_element) {
      elements->push_back(hashdb_element);
    }
  };


  public:
  typedef typename hashdb_elements_t::const_iterator const_iterator;

  // read dfxml into vector of elements
  dfxml_hashdigest_reader_manager_t(std::string p_dfxml_filename,
                                    std::string p_default_repository_name) :
           dfxml_filename(p_dfxml_filename),
           default_repository_name(p_default_repository_name),
           elements(0) {

    elements = new hashdb_elements_t();
    reader_consumer_t consumer(elements);

    dfxml_hashdigest_reader_t<reader_consumer_t, T>::do_read(
                                   dfxml_filename,
                                   default_repository_name,
                                   &consumer);
  }

  ~dfxml_hashdigest_reader_manager_t() {
    delete elements;
  }

  // element iterator access
  typename hashdb_elements_t::const_iterator begin() const {
    return elements->begin();
  }
  typename hashdb_elements_t::const_iterator end() const {
    return elements->end();
  }
};

#endif

