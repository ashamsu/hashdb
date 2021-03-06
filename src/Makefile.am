# Author:  Bruce Allen <bdallen@nps.edu>
# Created: 2/25/2013
#
# The software provided here is released by the Naval Postgraduate
# School, an agency of the U.S. Department of Navy.  The software
# bears no warranty, either expressed or implied. NPS does not assume
# legal liability nor responsibility for a User's use of the software
# or the results of such use.
#
# Please note that within the United States, copyright protection,
# under Section 105 of the United States Code, Title 17, is not
# available for any work of the United States Government and/or for
# any works created by United States Government employees. User
# acknowledges that this software contains work which was created by
# NPS government employees and is therefore in the public domain and
# not subject to copyright.
#
# Released into the public domain on February 25, 2013 by Bruce Allen.

# This file assists in building the hashdb_manager tool.

bin_PROGRAMS = hashdb
lib_LTLIBRARIES = libhashdb.la
libhashdb_la_LIBADD = liblmdb.la
noinst_LTLIBRARIES = liblmdb.la
liblmdb_la_SOURCES = \
	lmdb/libraries/liblmdb/lmdb.h \
	lmdb/libraries/liblmdb/mdb.c \
	lmdb/libraries/liblmdb/midl.h \
	lmdb/libraries/liblmdb/midl.c

# these CFLAGS are adapted from liblmdb/Makefile
liblmdb_la_CFLAGS = \
	-pthread -O3 -g \
	-W -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized
liblmdb_la_LDFLAGS = -static -PIC

hashdb_LDFLAGS = -static
libhashdb_la_LDFLAGS = -static -PIC -avoid-version

# to validate backward compatibility to c++98, add option -std=c++98
AM_CPPFLAGS = -isystem${top_srcdir}/src/misc \
	-isystem${top_srcdir}/src/btree/include \
	-isystem${top_srcdir}/src/endian/include \
	-I${top_srcdir}/src/lmdb/libraries/liblmdb \
	$(XML_CFLAGS)
AM_CFLAGS = $(HASHDB_CFLAGS)
AM_CXXFLAGS = $(HASHDB_CXXFLAGS)

hashdb_LDADD = libhashdb.la

HASHDB_INCS = \
	commands.hpp \
	dfxml_hashdigest_reader.hpp \
	dfxml_import_consumer.hpp \
	dfxml_scan_consumer.hpp \
	dfxml_scan_expanded_consumer.hpp \
	dfxml_hashdigest_writer.hpp \
	feature_file_reader.hpp \
	feature_line.hpp \
	json_formatter.hpp \
	main.cpp \
	progress_tracker.hpp \
	random_key.hpp \
	source_lookup_encoding.hpp \
	source_lookup_index_manager.hpp \
	source_metadata.hpp \
	source_metadata_element.hpp \
	source_metadata_manager.hpp \
	tab_hashdigest_reader.hpp \
	tcp_server_manager.hpp \
	usage.hpp

LIBHASHDB_INCS = \
	bi_data_types.hpp \
	bi_store.hpp \
	bloom_filter_manager.hpp \
	command_line.cpp \
	command_line.hpp \
	file_modes.h \
	globals.cpp \
	globals.hpp \
	hash_t_selector.h \
	hashdb_changes.hpp \
	hashdb.cpp \
	hashdb_directory_manager.hpp \
	hashdb_element.hpp \
	hashdb.hpp \
	hashdb_manager.hpp \
	hashdb_settings.hpp \
	hashdb_settings_reader.hpp \
	hashdb_settings_store.hpp \
	history_manager.hpp \
	lmdb_hash_store_iterator.hpp \
	lmdb_hash_store_manager.hpp \
	lmdb_resources_manager.hpp \
	lmdb_resources.h \
	logger.hpp \
	mutex_lock.hpp \
	tcp_client_manager.hpp

BTREE_INCS = \
	btree/include/boost/btree/index_helpers.hpp \
	btree/include/boost/btree/support/string_view.hpp \
	btree/include/boost/btree/support/random_string.hpp \
	btree/include/boost/btree/support/size_t_codec.hpp \
	btree/include/boost/btree/support/string_holder.hpp \
	btree/include/boost/btree/btree_index_set.hpp \
	btree/include/boost/btree/mmff.hpp \
	btree/include/boost/btree/btree_set.hpp \
	btree/include/boost/btree/btree_index_map.hpp \
	btree/include/boost/btree/helpers.hpp \
	btree/include/boost/btree/detail/binary_file.hpp \
	btree/include/boost/btree/detail/test_traits.hpp \
	btree/include/boost/btree/detail/index_bases.hpp \
	btree/include/boost/btree/detail/config.hpp \
	btree/include/boost/btree/detail/btree_bases.hpp \
	btree/include/boost/btree/detail/buffer_manager.hpp \
	btree/include/boost/btree/header.hpp \
	btree/include/boost/btree/bulk_load.hpp \
	btree/include/boost/btree/btree_map.hpp \
	btree/src/detail/buffer_manager.cpp \
	btree/src/detail/binary_file.cpp

ENDIAN_INCS = \
	endian/include/boost/endian/std_pair.hpp \
	endian/include/boost/endian/types.hpp \
	endian/include/boost/endian/detail/cover_operators.hpp \
	endian/include/boost/endian/detail/disable_warnings.hpp \
	endian/include/boost/endian/detail/config.hpp \
	endian/include/boost/endian/detail/disable_warnings_pop.hpp \
	endian/include/boost/endian/detail/intrinsic.hpp \
	endian/include/boost/endian/conversion.hpp

BLOOM_INCS = \
	misc/bloom.c \
	misc/bloom.h \
	misc/file_mapper.cpp \
	misc/file_mapper.hpp

DFXML_INCS = \
	dfxml/src/hash_t.h \
	dfxml/src/dfxml_writer.cpp \
	dfxml/src/dfxml_writer.h \
	dfxml/src/cppmutex.h

hashdb_SOURCES = \
	$(HASHDB_INCS)

libhashdb_la_SOURCES = \
	$(LIBHASHDB_INCS) \
	$(BTREE_INCS) $(ENDIAN_INCS) \
	$(LMDB_INCS) \
	$(BLOOM_INCS) $(DFXML_INCS)

include_HEADERS = hashdb.hpp

