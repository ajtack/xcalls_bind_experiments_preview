#ifndef _TXC_SENTINEL_H
#define _TXC_SENTINEL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>

#define TXC_SENTINEL_POOL_SIZE				128	
#define TXC_POOL_ELEMENT_FREE					1	
#define TXC_POOL_ELEMENT_ALLOCATED		2	

typedef struct txc_sentinel_s txc_sentinel_t;
typedef struct pool_element_sentinel_s pool_element_sentinel_t;
typedef struct txc_sentinelmgr_s txc_sentinelmgr_t;

struct txc_sentinel_s {
	pthread_rwlock_t rwl;
	int id;
	int owner;	
	unsigned int ref;
	unsigned int drop_it_on_commit; 
	unsigned int drop_it_on_abort; 
	unsigned int enlisted;
	txc_sentinelmgr_t *manager;
	pool_element_sentinel_t *pool_element;
};

struct pool_element_sentinel_s {
	txc_sentinel_t sentinel;
	unsigned int state;
	struct pool_element_sentinel_s *next;
	struct pool_element_sentinel_s *prev;
};


struct txc_sentinelmgr_s {
	pthread_mutex_t mutex;
	pool_element_sentinel_t *pool_free;
	pool_element_sentinel_t *pool_allocated;
	unsigned int pool_free_size;
	unsigned int pool_allocated_size;
};

TM_WAIVER txc_result_t txc_sentinelmgr_create(txc_sentinelmgr_t **sentinelmgrp);
TM_WAIVER txc_result_t txc_sentinel_create(txc_sentinelmgr_t *sentinelmgr, txc_sentinel_t **sentinelp);
TM_WAIVER void txc_sentinel_destroy(txc_sentinel_t **sentinel);
TM_WAIVER txc_result_t txc_sentinel_acquire_exclusive(txc_sentinel_t *sentinel, int block, int keep_list);
TM_WAIVER void txc_sentinel_acquire_shared(txc_sentinel_t *sentinel);
TM_WAIVER void txc_sentinel_release(txc_sentinel_t *sentinel);
TM_WAIVER void txc_sentinel_list_acquire_in_order();
TM_WAIVER void txc_sentinel_list_release(int commit_or_abort);
TM_WAIVER void txc_enlist_sentinel(txc_sentinel_t *sentinel, int acquired_status);
TM_WAIVER void txc_sentinel_drop_set(txc_sentinel_t *sentinel, int on_commit, int on_abort);
TM_WAIVER void txc_sentinel_list_sort();
TM_WAIVER void txc_sentinel_list_print();
#endif
