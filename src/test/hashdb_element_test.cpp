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
 * Test hashdb_element_t constructors.
 */

#include <config.h>
#include <cstdio>
#include "unit_test.h"
#include "hashdb_element.hpp"

void run_test() {

  hashdb_element_t element;
  TEST_EQ(element.hash_block_size, 0);
}

int main(int argc, char* argv[]) {
  run_test();
  return 0;
}

