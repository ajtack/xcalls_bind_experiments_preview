#ifndef _TXC_KOA_H
#define _TXC_KOA_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <txc/int/sentinel.h>
#include <txc/int/socket.h>
#include <txc/int/pipe.h>
#include <txc/int/pool.h>

#define TXC_KOA_TABLE_SIZE			256
#define TXC_KOA_INVALID					-1

#define TXC_KOA_IS_SOCK_DGRAM					1
#define TXC_KOA_IS_SOCK_STREAM				2
#define TXC_KOA_IS_PIPE_READ_END			3
#define TXC_KOA_IS_PIPE_WRITE_END			4
#define TXC_KOA_IS_FILE								5


/* Kernel Object Abstraction */
typedef struct txc_koa_s txc_koa_t;

struct txc_koa_s {
	pthread_mutex_t mutex;					/* mutex */
	int 						ref;						/* reference count */
	txc_sentinel_t	*sentinel;			/* user level sentinel */
	int							type;						/* type of object */ 	
	int 						synch_mode;			/* synchronization mode */
	union {
		txc_buffer_dgram_t 		buffer_input_dgram;
	};
	union {
		txc_buffer_dgram_t 		buffer_output_dgram;
	};
	union {
		txc_buffer_bytestream_t 		buffer_input_bytestream;
	};

	/* following are kernel-level info */
	dev_t 					st_dev; 				/* device */
	uid_t 					st_ino;					/* inode */
	dev_t						st_rdev;				/* device type (if inode device) */
}; 

typedef struct txc_koamgr_s txc_koamgr_t;

struct txc_koamgr_s {
	txc_koa_t *koa_tbl[TXC_KOA_TABLE_SIZE];
	txc_pool_t *pool_buffer_dgram;
	txc_pool_t *pool_buffer_bytestream;
	txc_pool_t *pool_koa_obj;
};

TM_WAIVER txc_result_t txc_koamgr_create(txc_koamgr_t **koamgrp);
TM_WAIVER void txc_koamgr_destroy(txc_koamgr_t **koamgrp);
TM_WAIVER txc_result_t txc_koa_create(txc_koamgr_t *koamgr, txc_koa_t **koap, int type);
TM_WAIVER void txc_koa_destroy(txc_koamgr_t *koamgr, txc_koa_t **koap);
TM_WAIVER txc_result_t txc_koa_get(txc_koamgr_t *koamgr, txc_koa_t **koap, int fd);
TM_WAIVER txc_result_t txc_koa_attach(txc_koamgr_t *koamgr, txc_koa_t *koa, int fd);
TM_WAIVER txc_result_t txc_koa_detach(txc_koamgr_t *koamgr, int fd);
#endif /* _TXC_KOA_H */
