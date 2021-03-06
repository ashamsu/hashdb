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
 * Test the bloom filter manager
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "to_key_helper.hpp"
#include "directory_helper.hpp"
#include "bloom_filter_manager.hpp"
#include "../hash_t_selector.h"
#include "file_modes.h"

static const char temp_dir[] = "temp_dir_bloom_filter_manager_test";
static const char temp[] = "temp_dir_bloom_filter_manager_test/bloom_filter_1";

void run_rw_test1() {

  hash_t key;

  remove(temp);

  bloom_filter_manager_t manager(std::string(temp_dir), RW_NEW, true, 28, 2);

  to_key(101, key);

  // enabled
  TEST_EQ(manager.is_positive(key), false);
  manager.add_hash_value(key);
  TEST_EQ(manager.is_positive(key), true);
}

void run_rw_test2() {

  hash_t key;

  remove(temp);

  bloom_filter_manager_t manager(std::string(temp_dir), RW_NEW, false, 28, 2);

  to_key(101, key);

  // manager is disabled
  TEST_EQ(manager.is_positive(key), true);
  manager.add_hash_value(key);
  TEST_EQ(manager.is_positive(key), true);
}

int main(int argc, char* argv[]) {
  make_dir_if_not_there(temp_dir);
  // validate that the is_positive function returns true when added
  run_rw_test1();
  // validate that the is_positive function returns true when disabled
  run_rw_test2();
  return 0;
}

