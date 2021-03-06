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
 * Provides the service of reading settings used by the hashdb.
 *
 * Throws std::runtime_error if the settings xml file is invalid.
 */

/**
 * NOTE: libxml2 uses type xmlChar which is unsigned char, which is
 * incompatible with string conversions.
 * All data in settings are and must remain simple ascii text.
 * This stipulation is easy to change, see dfxml_hashdigest_reader.hpp.
 */

#ifndef HASHDB_SETTINGS_READER_HPP
#define HASHDB_SETTINGS_READER_HPP
#ifdef WIN32
  #include "io.h"
#endif
#include <libxml/parser.h>
#include "hashdb_settings.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>

/**
 * \file
 * This file reads hashdb settings.
 */

// a class is used just to keep members private
class hashdb_settings_reader_t {
  private:

  // do not allow these
  hashdb_settings_reader_t();
  hashdb_settings_reader_t(const hashdb_settings_reader_t&);
  hashdb_settings_reader_t& operator=(const hashdb_settings_reader_t&);

  // ************************************************************
  // nodes
  // ************************************************************

  // nodes
  enum node_type_t {NO_NODE,
                    // hashdb
                    SETTINGS_VERSION,
                    HASH_DIGEST_TYPE,
                    BYTE_ALIGNMENT,
                    HASH_BLOCK_SIZE,
                    MAXIMUM_HASH_DUPLICATES,
                    BLOOM1_USED,
                    BLOOM1_K_HASH_FUNCTIONS,
                    BLOOM1_M_HASH_SIZE,
                    BLOOM2_USED,
                    BLOOM2_K_HASH_FUNCTIONS,
                    BLOOM2_M_HASH_SIZE};

  // ************************************************************
  // user data type for sax
  // ************************************************************
  struct user_data_t {

    // parser state
    hashdb_settings_t* settings;
    node_type_t active_node;
    size_t index;

    user_data_t(hashdb_settings_t* p_settings) : settings(p_settings),
                    active_node(NO_NODE),
                    index(0) {
    }

    // don't allow these
    private:
    user_data_t(const user_data_t&);
    user_data_t& operator=(const user_data_t&);
  };

  // ************************************************************
  // static sax handler helper functions
  // ************************************************************
  // xmlChar to string
  static void xmlChar_to_string(const xmlChar* characters, int len, std::string& s) {
    char temp_chars[len+1];
    strncpy(temp_chars, (const char*)characters, len);
    temp_chars[len] = (char)NULL;
    s = std::string(temp_chars);
  }

  // convert node name to node type
  static node_type_t xmlChar_to_node_type(const xmlChar* name) {
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("settings_version"))) return SETTINGS_VERSION;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hash_digest_type"))) return HASH_DIGEST_TYPE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("byte_alignment"))) return BYTE_ALIGNMENT;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("hash_block_size"))) return HASH_BLOCK_SIZE;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("maximum_hash_duplicates"))) return MAXIMUM_HASH_DUPLICATES;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("bloom1_used"))) return BLOOM1_USED;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("bloom1_k_hash_functions"))) return BLOOM1_K_HASH_FUNCTIONS;
    if (xmlStrEqual(name, reinterpret_cast<const xmlChar*>("bloom1_M_hash_size"))) return BLOOM1_M_HASH_SIZE;
    return NO_NODE;
  }

  // convert string to number or fail with exit
  template <class T>
  static void xmlChar_to_number(const xmlChar* c, int len, T& number) {
    std::string number_string;
    xmlChar_to_string(c, len, number_string);
    try {
      number = boost::lexical_cast<T>(number_string);
      return;
    } catch(...) {
      // abort
      std::ostringstream s;
      s << "Invalid number when reading settings: '"
        << number_string << "'";
      throw std::runtime_error(s.str());
    }
  }

  __attribute__((noreturn)) static void exit_invalid_state(std::string message) {
    std::ostringstream s;
    s << message;
    throw std::runtime_error(s.str());
  }

  __attribute__((noreturn)) static void exit_invalid_text(std::string message, std::string text) {
    std::ostringstream s;
    s << message << ": '" << text << "'.";
    throw std::runtime_error(s.str());
  }

  __attribute__((noreturn)) static void exit_invalid_index(size_t index) {
    std::ostringstream s;
    s << "Invalid bloom filter index " << index << ".";
    throw std::runtime_error(s.str());
  }

  // ************************************************************
  // static sax handlers
  // ************************************************************
  // these on_* methods implement SAX parsing
  static void on_start_document(void* p_user_data ATTRIBUTE_UNUSED) {
    // no action
  }
  static void on_end_document(void* p_user_data ATTRIBUTE_UNUSED) {
    // no action
  }

  static void on_start_element(void* p_user_data,
                               const xmlChar* name,
                               const xmlChar** attributes) {

    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    // identify active node type
    user_data.active_node = xmlChar_to_node_type(name);
  }

  static void on_end_element(void* p_user_data,
                             const xmlChar* name) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));
    user_data.active_node = NO_NODE;
  }

  static void on_characters(void* p_user_data,
                            const xmlChar* characters,
                            int len) {
    user_data_t& user_data = *(static_cast<user_data_t*>(p_user_data));

    bool is_valid;

    if (user_data.active_node == SETTINGS_VERSION) {
      xmlChar_to_number(characters, len, user_data.settings->settings_version);
    } else if (user_data.active_node == HASH_DIGEST_TYPE) {
      xmlChar_to_string(characters, len, user_data.settings->hash_digest_type);
    } else if (user_data.active_node == BYTE_ALIGNMENT) {
      xmlChar_to_number(characters, len, user_data.settings->byte_alignment);
    } else if (user_data.active_node == HASH_BLOCK_SIZE) {
      xmlChar_to_number(characters, len, user_data.settings->hash_block_size);
    } else if (user_data.active_node == MAXIMUM_HASH_DUPLICATES) {
      xmlChar_to_number(characters, len, user_data.settings->maximum_hash_duplicates);

    } else if (user_data.active_node == BLOOM1_USED) {
      std::string bloom1_state_string;
      xmlChar_to_string(characters, len, bloom1_state_string);
      is_valid = string_to_bloom_state(bloom1_state_string, user_data.settings->bloom1_is_used);
      is_valid = string_to_bloom_state(bloom1_state_string, user_data.settings->bloom1_is_used);
      if (!is_valid) {
        exit_invalid_state("Error: invalid bloom 1 selection\n");
      }

    } else if (user_data.active_node == BLOOM1_K_HASH_FUNCTIONS) {
      uint32_t k;
      xmlChar_to_number(characters, len, k);
      user_data.settings->bloom1_k_hash_functions = k;

    } else if (user_data.active_node == BLOOM1_M_HASH_SIZE) {
      uint32_t M;
      xmlChar_to_number(characters, len, M);
      user_data.settings->bloom1_M_hash_size = M;
    }
  }

  static void on_comment(void* p_user_data ATTRIBUTE_UNUSED) {
  //    std::cout << "hashdb_settings_reader_t::on_comment " << text << "\n";
  }

  static void on_warning(void* p_user_data ATTRIBUTE_UNUSED,
                         const char* msg,
                         ...) {
    // adapted from http://stackoverflow.com/questions/5977326/call-printf-using-va-list
    va_list arglist;
    printf("settings_reader on_warning(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "settings_reader on_warning(): " << msg << std::endl;

  }

  static void on_error(void* p_user_data ATTRIBUTE_UNUSED,
                       const char* msg,
                       ...) {
    va_list arglist;
    printf("settings_reader on_error(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "settings_reader on_error(): " << msg << std::endl;
  }

  static void on_fatal_error(void* p_user_data ATTRIBUTE_UNUSED,
                             const char* msg,
                             ...) {
    va_list arglist;
    printf("settings_reader on_fatal_error(): ");
    va_start(arglist, msg);
    vprintf(msg, arglist);
    va_end(arglist);
//    std::cout << "settings_reader on_fatal_error(): " << msg << std::endl;
  }

  public:
  /**
   * read onto default hashdb settings or throw std::runtime_error.
   */
  static void read_settings(const std::string filename,
                            hashdb_settings_t& settings) {

    // verify that the settings file exists
    bool file_is_present = (access(filename.c_str(),F_OK) == 0);
    if (!file_is_present) {
      std::ostringstream ss3;
      ss3 << "Settings file '" << filename << "' does not exist.";
      throw std::runtime_error(ss3.str());
    }

    // set up the sax callback data structure with context-relavent handlers
    xmlSAXHandler sax_handlers = {
      NULL,			// internalSubset
      NULL,			// isStandalone
      NULL,			// hasInternalSubset
      NULL,			// hasExternalSubset
      NULL,			// resolveEntity
      NULL,			// getEntity
      NULL,			// entityDecl
      NULL,			// notationDecl
      NULL,			// attributeDecl
      NULL,			// elementDecl
      NULL,			// unparsedEntityDecl
      NULL,			// setDocumentLocator
      on_start_document,	// startDocument
      on_end_document,		// endDocument
      on_start_element,		// startElement
      on_end_element,		// endElement
      NULL,			// reference
      on_characters,		// characters
      NULL,			// ignorableWhitespace
      NULL,			// processingInstruction
      NULL,			// comment
      on_warning,		// xmlParserWarning
      on_error,			// xmlParserError
      on_fatal_error,		// xmlParserFatalError
      NULL,			// getParameterEntity
      NULL,			// cdataBlock
      NULL,			// externalSubset
      1     			// is initialized
    };

    // set up the data structure for the sax handlers to use
    user_data_t user_data(&settings);

    // perform the sax parse on the file
    int sax_parser_resource = xmlSAXUserParseFile(
                               &sax_handlers, &user_data, filename.c_str());
    if (sax_parser_resource == 0) {
      // good, no failure, settings is now read in
      xmlCleanupParser(); // mostly for valgrind
      
      return;
    } else {
      // something went wrong
      std::ostringstream ss4;
      ss4 << "malformed settings in file '" << filename << "'.";
      throw std::runtime_error(ss4.str());
    }
  }
};

#endif

