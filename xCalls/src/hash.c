/* Copyright 2006 David Crawshaw, released under the new BSD license.
 * Version 2, from http://www.zentus.com/c/hash.html */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <txc/int/hash.h>

/* Table is sized by primes to minimise clustering.
   See: http://planetmath.org/encyclopedia/GoodHashTablePrimes.html */
static const unsigned int sizes[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};
static const int sizes_count = sizeof(sizes) / sizeof(sizes[0]);
static const float load_factor = 0.65;

/*************************************************************************
 ***                    INTEGER Key Implementation                     ***
 *************************************************************************/

struct txc_hash_int_record_s {
    unsigned int key;
    void *value;
};

struct txc_hash_int_s {
    struct record *records;
    unsigned int records_count;
    unsigned int size_index;
};

static int 
hash_int_grow(txc_hash_int_t *h)
{
    int i;
    struct txc_hash_int_record_s *old_recs;
    unsigned int old_recs_length;

    old_recs_length = sizes[h->size_index];
    old_recs = h->records;

    if (h->size_index == sizes_count - 1) return -1;
    if ((h->records = calloc(sizes[++h->size_index],
                    sizeof(struct txc_hash_int_record_s))) == NULL) {
        h->records = old_recs;
        return -1;
    }

    h->records_count = 0;

    // rehash table
    for (i=0; i < old_recs_length; i++)
        if (old_recs[i].key) 
            hash_int_add(h, old_recs[i].key, old_recs[i].value);

    free(old_recs);

    return 0;
}


txc_result_t
txc_hash_int_create(txc_hash_int_t **hp, unsigned int capacity) {
    struct txc_hash_int_s *h;
    int i, sind;

    capacity /= load_factor;

    for (i=0; i < sizes_count; i++) 
        if (sizes[i] > capacity) { sind = i; break; }

    if ((h = malloc(sizeof(struct txc_hash_int_s))) == NULL) {
			return TXC_R_NOMEMORY;
		}
    if ((h->records = calloc(sizes[sind], sizeof(struct txc_hash_int_record_s))) == NULL) {
        free(h);
        return TXC_R_NOMEMORY;
    }

    h->records_count = 0;
    h->size_index = sind;

    *hp = h;
		return TXC_R_SUCCESS;
}

void
txc_hash_int_destroy(txc_hash_int_t **hp)
{
  free((*hp)->records);
  free(*hp);
	*hp = NULL;	
}

txc_result_t 
txc_hash_int_add(txc_hash_int_t *h, unsigned int key, void *value)
{
    struct txc_hash_int_record_s *recs;
    int rc;
    unsigned int off, ind, size, code;

    if (key == 0x0) return -2;
    if (h->records_count > sizes[h->size_index] * load_factor) {
				return TXC_R_NOMEMORY;
				//FIXME: can we grow the hash table in a transactional environment?
        //rc = hash_int_grow(h);
        //if (rc) return rc;
    }

    code = key;
    recs = h->records;
    size = sizes[h->size_index];

    ind = code % size;
    off = 0;

    while (recs[ind].key)
        ind = (code + (int)pow(++off,2)) % size;

    recs[ind].key = key;
    recs[ind].value = value;

    h->records_count++;

    return TXC_R_SUCCESS;
}

void * 
txc_hash_int_get(txc_hash_int_t *h, unsigned int key)
{
    struct txc_hash_int_record_s *recs;
    unsigned int off, ind, size;
    unsigned int code = key;

    recs = h->records;
    size = sizes[h->size_index];
    ind = code % size;
    off = 0;

    // search on hash which remains even if a record has been removed,
    // so hash_int_remove() does not need to move any collision records
    while (recs[ind].key) {
        if (key == recs[ind].key) {
            return recs[ind].value;
				}
        ind = (code + (int)pow(++off,2)) % size;
    }

    return NULL;
}

void * 
txc_hash_int_remove(txc_hash_int_t *h, unsigned int key)
{
    unsigned int code = key;
    struct txc_hash_int_record_s *recs;
    void * value;
    unsigned int off, ind, size;

    recs = h->records;
    size = sizes[h->size_index];
    ind = code % size;
    off = 0;

    while (recs[ind].key) {
        if (key == recs[ind].key) {
            // do not erase hash, so probes for collisions succeed
            value = recs[ind].value;
            recs[ind].key = 0;
            recs[ind].value = 0;
            h->records_count--;
            return value;
        }
        ind = (code + (int)pow(++off, 2)) % size;
    }
 
    return NULL;
}

unsigned int 
txc_hash_int_size(txc_hash_int_t *h)
{
    return h->records_count;
}



/*************************************************************************
 ***							      STRING Key Implementation										   ***
 *************************************************************************/


struct txc_hash_str_record_s {
    unsigned int hash;
    char *key;
    void *value;
};

struct txc_hash_str_s {
    struct record *records;
    unsigned int records_count;
    unsigned int size_index;
};



static int 
hash_str_grow(txc_hash_str_t *h)
{
    int i;
    struct txc_hash_str_record_s *old_recs;
    unsigned int old_recs_length;

    old_recs_length = sizes[h->size_index];
    old_recs = h->records;

    if (h->size_index == sizes_count - 1) return -1;
    if ((h->records = calloc(sizes[++h->size_index],
                    sizeof(struct txc_hash_str_record_s))) == NULL) {
        h->records = old_recs;
        return -1;
    }

    h->records_count = 0;

    // rehash table
    for (i=0; i < old_recs_length; i++)
        if (old_recs[i].hash && old_recs[i].key)
            hash_str_add(h, old_recs[i].key, old_recs[i].value);

    free(old_recs);

    return 0;
}

/* algorithm djb2 */
static unsigned int 
strhash(const char *str)
{
    int c;
    int hash = 5381;
    while (c = *str++)
        hash = hash * 33 + c;
    return hash == 0 ? 1 : hash;
}


txc_result_t
txc_hash_str_create(txc_hash_str_t **hp, unsigned int capacity) {
    struct txc_hash_str_s *h;
    int i, sind;

    capacity /= load_factor;

    for (i=0; i < sizes_count; i++) 
        if (sizes[i] > capacity) { sind = i; break; }

    if ((h = malloc(sizeof(struct txc_hash_str_s))) == NULL) {
			return TXC_R_NOMEMORY;
		}
    if ((h->records = calloc(sizes[sind], sizeof(struct txc_hash_str_record_s))) == NULL) {
        free(h);
        return TXC_R_NOMEMORY;
    }

    h->records_count = 0;
    h->size_index = sind;

    *hp = h;
		return TXC_R_SUCCESS;
}

void
txc_hash_str_destroy(txc_hash_str_t **hp)
{
  free((*hp)->records);
  free(*hp);
	*hp = NULL;	
}

txc_result_t 
txc_hash_str_add(txc_hash_str_t *h, char *key, void *value)
{
    struct txc_hash_str_record_s *recs;
    int rc;
    unsigned int off, ind, size, code;

    if (key == NULL || *key == '\0') return -2;
    if (h->records_count > sizes[h->size_index] * load_factor) {
				return TXC_R_NOMEMORY;
				//FIXME: can we grow the hash table in a transactional environment?
        //rc = hash_str_grow(h);
        //if (rc) return rc;
    }

    code = strhash(key);
    recs = h->records;
    size = sizes[h->size_index];

    ind = code % size;
    off = 0;

    while (recs[ind].key)
        ind = (code + (int)pow(++off,2)) % size;

    recs[ind].hash = code;
    recs[ind].key = key;
    recs[ind].value = value;

    h->records_count++;

    return TXC_R_SUCCESS;
}

void * 
txc_hash_str_get(txc_hash_str_t *h, char *key)
{
    struct txc_hash_str_record_s *recs;
    unsigned int off, ind, size;
    unsigned int code = strhash(key);

    recs = h->records;
    size = sizes[h->size_index];
    ind = code % size;
    off = 0;

    // search on hash which remains even if a record has been removed,
    // so hash_str_remove() does not need to move any collision records
    while (recs[ind].hash) {
        if ((code == recs[ind].hash) && recs[ind].key &&
                strcmp(key, recs[ind].key) == 0)
            return recs[ind].value;
        ind = (code + (int)pow(++off,2)) % size;
    }

    return NULL;
}

void * 
txc_hash_str_remove(txc_hash_str_t *h, char *key)
{
    unsigned int code = strhash(key);
    struct txc_hash_str_record_s *recs;
    void * value;
    unsigned int off, ind, size;

    recs = h->records;
    size = sizes[h->size_index];
    ind = code % size;
    off = 0;

    while (recs[ind].hash) {
        if ((code == recs[ind].hash) && recs[ind].key &&
                strcmp(key, recs[ind].key) == 0) {
            // do not erase hash, so probes for collisions succeed
            value = recs[ind].value;
            recs[ind].key = 0;
            recs[ind].value = 0;
            h->records_count--;
            return value;
        }
        ind = (code + (int)pow(++off, 2)) % size;
    }
 
    return NULL;
}

unsigned int 
txc_hash_str_size(txc_hash_str_t *h)
{
    return h->records_count;
}
