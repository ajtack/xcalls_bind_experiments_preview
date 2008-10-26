/* Copyright 2006 David Crawshaw, released under the new BSD license.
 * Version 2, from http://www.zentus.com/c/hash.html */

#ifndef _TXC_HASH_H
#define _TXC_HASH_H

#include <txc/int/result.h>

/* Opaque structure used to represent hashtable. */
typedef struct txc_hash_int_s txc_hash_int_t;
typedef struct txc_hash_str_s txc_hash_str_t;

/* Create new hashtable. */
txc_result_t txc_hash_int_create(txc_hash_int_t **hp, unsigned int capacity);
txc_result_t txc_hash_str_create(txc_hash_str_t **hp, unsigned int capacity);

/* Free hashtable. */
void txc_hash_int_destroy(txc_hash_int_t **hp);
void txc_hash_str_destroy(txc_hash_str_t **hp);

/* Add key/value pair. Returns non-zero value on error (eg out-of-memory). */
txc_result_t txc_hash_int_add(txc_hash_int_t *h, unsigned int key, void *value);
txc_result_t txc_hash_str_add(txc_hash_str_t *h, char * key, void *value);

/* Return value matching given key. */
void * txc_hash_int_get(txc_hash_int_t *h, unsigned int key);
void * txc_hash_str_get(txc_hash_str_t *h, char * key);

/* Remove key from table, returning value. */
void * txc_hash_int_remove(txc_hash_int_t *h, unsigned int key);
void * txc_hash_str_remove(txc_hash_str_t *h, char * key);

/* Returns total number of keys in the hashtable. */
unsigned int txc_hash_int_size(txc_hash_int_t *h);
unsigned int txc_hash_str_size(txc_hash_str_t *h);

#endif
