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
 * Test the libhashdb interfaces.
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "hashdb.hpp"
#include "to_key_helper.hpp"
#include "directory_helper.hpp"
#include "../hash_t_selector.h"
#include "source_metadata.hpp"

static const char temp_dir[] = "temp_dir_libhashdb_test.hdb";

typedef hashdb_t__<hash_t> hashdb_t;

void do_import() {
  // clean up from any previous run
  rm_hashdb_dir(temp_dir);

  // valid hashdigest values
  hash_t k1;
  to_key(0, k1);

  // input for import
  hashdb_t::import_input_t import_input;
  import_input.push_back(hashdb_t::import_element_t(k1, "rep1", "file1", 0));
  import_input.push_back(hashdb_t::import_element_t(k1, "rep1", "file1", 4096));
  import_input.push_back(hashdb_t::import_element_t(k1, "rep1", "file1", 4097)); // invalid

  // create new database
  hashdb_t hashdb;
  std::pair<bool, std::string> import_pair = hashdb.open_import(temp_dir, 4096, 20);
  TEST_EQ(import_pair.first, true);

  // import some elements
  int status;
  status = hashdb.import(import_input);
  TEST_EQ(status, 0);

  // import metadata
  hash_t k2;
  to_key(1, k2);
  status = hashdb.import_metadata("rep1", "file1", 10000, k2);
  TEST_EQ(status, 0);
  status = hashdb.import_metadata("zrep1", "file1", 10000, k2);
  TEST_EQ(status, 0);
  status = hashdb.import_metadata("zrep1", "file1", 10000, k2);
  TEST_EQ(status, 0);

  // invalid mode to scan when importing
  hashdb_t::scan_input_t scan_input;
  hashdb_t::scan_output_t scan_output;
  std::cout << "may emit scan mode error here.\n";
  status = hashdb.scan(scan_input, scan_output);
  TEST_NE(status, 0);
}

void do_scan() {

  // valid hashdigest values
  hash_t k1;
  hash_t k2;
  to_key(0, k1);
  to_key(0, k2);

  // open to scan
  hashdb_t hashdb;
  std::pair<bool, std::string> open_pair = hashdb.open_scan(temp_dir);
  TEST_EQ(open_pair.first, true);

  hashdb_t::scan_input_t input;
  hashdb_t::scan_output_t output;

  // populate input
  input.push_back(k1);
  input.push_back(k2);

  // perform scan
  hashdb.scan(input, output);
  TEST_EQ(output.size(), 2);
  TEST_EQ(output[0].first, 0);
  TEST_EQ(output[0].second, 2);
  TEST_EQ(output[1].first, 1);
  TEST_EQ(output[0].second, 2);
}

int main(int argc, char* argv[]) {
  do_import();
  do_scan();

  return 0;
}

