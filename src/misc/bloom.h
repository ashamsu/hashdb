/*
 * bloom.h:
 * 
 * bloom filter for NIST NSRL
 * Originally (C) August 2006, Simson L. Garfinkel
 * Released into the public domain in September 2008.
 * 
 * You must run the following autoconf macros in the configure file:
 * AC_TYPE_INT64_T
 * AC_CHECK_HEADERS([openssl/hmac.h openssl/pem.h])
 * AC_CHECK_FUNCS([printf getrusage err errx warn warnx mmap])
 * AC_CHECK_HEADERS([err.h sys/mman.h sys/resource.h unistd.h])
 */

#ifndef NSRL_BLOOM_H
#define NSRL_BLOOM_H

#ifndef PACKAGE_VERSION
#error must include autoconf config.h file
#endif

/* This must appear before inttypes.h is included */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "file_mapper.hpp"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <sys/types.h>

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifndef __BEGIN_DECLS
#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif
/* End Win32 */


__BEGIN_DECLS

/* Calculate the bloom filter's false positive rate given:
 *  m = Number of slots in the table.
 *  n = Number of elements stored in the table.
 *  k = Number of hash functions.
 */
#define BLOOM_CALC_P(m,n,k) pow(1-exp((-(double)(k)*n)/(m)),(k))

typedef struct  nsrl_bloom_ {
    map_impl_t* map_handle;	// handle to boost map implementation
    uint32_t hash_bytes;	// hash_bits/8 ; performance optimization
    uint32_t M;	                // number of bits of hash to use for each bloom function (log2(vector_bytes))
    uint32_t k;	                // number of bloom functions to use
    size_t   vector_bytes;	// length of the vector (vector_bytes*8 = 2^M)
    uint32_t vector_offset;	// offset of vector in file (usually 4096)
    uint8_t *vector;		// the bloom filter
    char    *comment;
    uint64_t added_items;       // How many times add() was called.
    uint64_t unique_added_items;      // Times add() set all k bits
    uint64_t aliased_adds;      // Times add() set 0 bits
    int   fd;			// file descriptor for mapped or open file
    char* fname;                // stores char* of path to file
    uint32_t debug;		// debug level
    uint32_t memmapped:1;	// true if we need to unmap; otherwise we free(vector)
    uint32_t free_this:1;	// should we free this on clean?
    uint32_t fileio:1;		// force file i/o
    uint64_t hits;		// stats
#ifdef HAVE_PTHREAD
    uint32_t multithreaded:1;	// are we multithreaded?
    pthread_mutex_t mutex;
#endif
} nsrl_bloom;



/* Service functions for hexdecimal conversion: */
int nsrl_hex2bin(uint8_t *binbuf,size_t binbuf_size, const char *hex);
const char *nsrl_hexbuf(char *dst,size_t dst_len,const uint8_t *bin,uint32_t bytes,int flag);

#define NSRL_HEXBUF_UPPERCASE 0x01
#define NSRL_HEXBUF_SPACE2    0x02
#define NSRL_HEXBUF_SPACE4    0x04

/** nsrl_bloom_alloc(): allocates the filter memory. */
nsrl_bloom *nsrl_bloom_alloc(void);		// allocate a new bloom object

/** nsrl_bloom_create:
 * Create a nsrl bloom filter for either a 128-bit or a 160-bit hash.
 * If fname is specified, the file is created; otherwise the bloom
 * filter is kept in memory. M is log2(m); k is the number of functions to use.
 * Returns 0 if successful, -1 if failure.
 */
int nsrl_bloom_create(nsrl_bloom *b, const char *fname, uint32_t hash_bits, uint32_t bloom_bits, 
                      uint32_t k, const char *comment);

#ifdef HAVE_PTHREAD
/** nsrl_bloom_init_mutex:
 * Make this bloom filter multi-threaded (if mutext support is available).
 * Returns 0 if successful, -1 if failure.
 */
int nsrl_bloom_init_mutex(nsrl_bloom *b);
#endif


/** nsrl_bloom_open: 
 * Opens an existing bloom filter, taking the parameters from it.
 * Returns 0 if successful, -1 if failure.
 */
int nsrl_bloom_open(nsrl_bloom *b,const char *fname, map_permissions_t fileMode);	// open an existing filter

int nsrl_bloom_write(nsrl_bloom *b,const char *fname);	// open an existing filter

/* add and query */

void nsrl_bloom_fprint_info(const nsrl_bloom *b,FILE *out);
void nsrl_bloom_print_info(const nsrl_bloom *b);
void nsrl_bloom_fprint_usage(FILE *out);
void nsrl_bloom_print_usage(void);
void nsrl_bloom_add(nsrl_bloom *b,const uint8_t *hash); 
int  nsrl_bloom_query( nsrl_bloom *b,const uint8_t *hash); // 0 not present; 1 is present
double nsrl_bloom_utilization(const nsrl_bloom *b);			  // on scale 0..1

void  nsrl_bloom_clear(nsrl_bloom *b);	// Release resources and sanitize
void  nsrl_bloom_free(nsrl_bloom *b);	// free *b

/* debug */
void nsrl_bloom_info(char *buf,size_t buflen,const nsrl_bloom *b);	
void nsrl_calc_histogram(const nsrl_bloom *b,uint64_t counts[256]);
void nsrl_print_histogram(const nsrl_bloom *b,const uint64_t counts[256]);


__END_DECLS

#ifdef __cplusplus
#include <string>
class NSRLBloom : public nsrl_bloom {
public:
    NSRLBloom(){
        this->map_handle = 0;
	this->hash_bytes = 0;
	this->M = 0;
	this->k = 0;
	this->vector_bytes = 0;
	this->vector_offset = 0;
	this->vector = 0;
	this->comment = 0;
	this->added_items = 0;
	this->unique_added_items = 0;
	this->aliased_adds = 0;
	this->fd = 0;
        this->fname = 0;
	this->debug = 0;
	this->memmapped=0;
	this->free_this=0;
	this->hits = 0;
#ifdef HAVE_PTHREAD
	this->multithreaded=0;
#endif
	nsrl_bloom_clear(this);
    };
    /** Open the bloom filter; return 0 if sucessful. */
    int open(const char *fname, map_permissions_t fileMode){
	return nsrl_bloom_open(this,fname, fileMode);
    }

    int create(const char *fname, uint32_t hash_bits, uint32_t bloom_bits,
               uint32_t k, const char *comment) {  // k: hashesPerMd5...
	return nsrl_bloom_create(this, fname, hash_bits, bloom_bits, k, comment);
    }
#ifdef HAVE_PTHREAD
    int init_mutex() {return nsrl_bloom_init_mutex(this); }
#endif
    void print_info() const {	nsrl_bloom_print_info(this);     }

    void fprint_info(FILE *out) const { 	nsrl_bloom_fprint_info(this,out);    }

    void add(const uint8_t *hash){	nsrl_bloom_add(this,hash);    }
    bool query(const uint8_t *hash) const {	return nsrl_bloom_query(const_cast<NSRLBloom*>(this),hash);    }
    double utilization() const{
	return nsrl_bloom_utilization(this);
    }
    uint64_t calchits(){
	return this->hits;
    }
    int write(const char *fname) {
	return nsrl_bloom_write(this, fname);
    }

    static void print_usage() {nsrl_bloom_print_usage(); }
    static void fprint_usage(FILE *out) {nsrl_bloom_fprint_usage(out); }

    ~NSRLBloom(){
	nsrl_bloom_clear(this);
    }
};
#endif
#endif
