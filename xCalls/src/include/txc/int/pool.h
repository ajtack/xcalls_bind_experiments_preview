#ifndef _TXC_POOL_H
#define _TXC_POOL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>

#define TXC_POOL_OBJECT_FREE					0x1	
#define TXC_POOL_OBJECT_ALLOCATED			0x2	

typedef struct txc_pool_object_s txc_pool_object_t;
typedef struct txc_pool_s txc_pool_t;
typedef void (*txc_pool_object_constructor_t)(void *buf); 
typedef void (*txc_pool_object_print_t)(void *buf); 

struct txc_pool_object_s {
	void * buf;
	char status;
	struct txc_pool_object_s *next;
	struct txc_pool_object_s *prev;
};

struct txc_pool_s {
	pthread_mutex_t mutex;
	char mtsafe;
	void *full_buf;			
	txc_pool_object_t *obj_list;
	txc_pool_object_t *obj_free;
	txc_pool_object_t *obj_allocated;
	unsigned int obj_free_num;
	unsigned int obj_allocated_num;
	unsigned int obj_size;
	unsigned int obj_num;
	txc_pool_object_constructor_t obj_constructor;
};

TM_WAIVER txc_result_t txc_pool_create(txc_pool_t **poolp, unsigned int obj_size,
                    unsigned int obj_num, txc_pool_object_constructor_t obj_constructor);
TM_WAIVER void txc_pool_destroy(txc_pool_t **poolp);
TM_WAIVER txc_result_t txc_pool_object_alloc(txc_pool_t *pool, void **objp);
TM_WAIVER void txc_pool_object_free(txc_pool_t *pool, void **objp);
void txc_pool_print(txc_pool_t *pool, txc_pool_object_print_t printer);

#endif
