#ifndef _TXC_TRANSACTION_H 
#define _TXC_TRANSACTION_H 

/* Define to 1 if building debugging support for KOA module */
#undef _TXC_KOA_DEBUG

typedef struct txc_xact_descriptor_s txc_xact_descriptor_t;
typedef struct txc_transactionmgr_s txc_transactionmgr_t;
typedef struct txc_transaction_log_record_s txc_transaction_log_record_t;
typedef struct txc_transaction_log_record_xact_frame_s txc_transaction_log_record_xact_frame_t;
typedef struct txc_transaction_log_record_sentinel_s txc_transaction_log_record_sentinel_t;
typedef struct txc_transaction_log_record_commit_action_s txc_transaction_log_record_commit_action_t;
typedef struct txc_transaction_log_record_compensating_action_s txc_transaction_log_record_compensating_action_t;

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include <ia32intrin.h>

#define TXC_FPRINTFOUT						stdout

#define TXC_MAX_NUM_THREADS						32		
#define XACT_MAX_NUM_ACTION_ARGS 			6
#define XACT_MAX_NUM_ACTIONS 					10
#define XACT_MAX_NUM_SENTINELS				10 
#define TXC_MAX_NUM_LOG_RECORDS			128
#define TXC_MAX_SIZE_LOG_BUFFER				256*1024

#define TXC_LOG_RECORD_FRAME									0
#define TXC_LOG_RECORD_COMMIT_ACTION					1
#define TXC_LOG_RECORD_COMPENSATING_ACTION		2
#define TXC_LOG_RECORD_SENTINEL								3

#define TXC_CACHE_LINE_SIZE										64

#define _TXC_LOGGER_H_PRIVATE_INTERFACE
#include <txc/int/result.h>
#include <txc.h>
#include <txc/int/stat.h>
#include <txc/int/logger.h>
#include <txc/int/koa.h>

/*
 * XCALLS MODEL DESCRIPTION
 * --------------------------
 * - Before doing a non-protected library call we need to protect it using a
 *   sentinel
 *    + Use SENTINEL_ACQUIRE_SHARED for non-transactional code execting a call
 *    + Use SENTINEL_ACQUIRE_EXCLUSIVE for transactional code
 * - When trying to acquire a sentinel inside a transaction the following can
 *	 happen:
 *    + sentinel is free      => acquisition by the transaction 
 *    + sentinel is not-free 	=> transaction aborts and restarts (and releases
 *      all sentinels to avoid deadlock. We do this because we cannot detect
 *      deadlocks) 
 *		  Optimization: On restart re-acquire all sentinels in-order before 
 * 			proceeding.
 *
 * COMMIT/COMPENSATING ACTIONS
 * ---------------------------
 * - Register commit/compensating actions using:
 *    + register_commit_action
 *    + register_compensating_action
 *   Must pass the following parameters in this order:
 *    + function pointer to the action
 *    + number of arguments
 *    + table of arguments
 * - Sentinels are released after executing all the commit actions (on commit)
 *   or all the compensating actions (on abort)
 */


TM_WAIVER int pthread_rwlock_trywrlock(pthread_rwlock_t *);

#ifdef _TXC_TRANSACTION_H_INCLUDE_PRIVATE
struct txc_transaction_log_record_xact_frame_s {
	txc_transaction_log_record_sentinel_t *sentinel_list_head;
	txc_transaction_log_record_sentinel_t *sentinel_list_tail;
	int has_commit_actions;
	int has_compensating_actions;
};

struct txc_transaction_log_record_commit_action_s {
	txc_result_t (*action)(unsigned int arg_num, unsigned int *arg_list);
	unsigned int num_args;
	unsigned int arg_list[XACT_MAX_NUM_ACTION_ARGS];  
};

struct txc_transaction_log_record_compensating_action_s {
	txc_result_t (*action)(unsigned int arg_num, unsigned int *arg_list);
	unsigned int num_args;
	unsigned int arg_list[XACT_MAX_NUM_ACTION_ARGS];  
};

struct txc_transaction_log_record_sentinel_s {
	txc_sentinel_t *ptr;
	int acquired_status;
	struct txc_transaction_log_record_sentinel_s *next;
	struct txc_transaction_log_record_sentinel_s *prev;
};

struct txc_transaction_log_record_s {
	unsigned int type;
	union {
		txc_transaction_log_record_xact_frame_t 						xact_frame;
		txc_transaction_log_record_commit_action_t 					commit_action;
		txc_transaction_log_record_compensating_action_t 		compensating_action;
		txc_transaction_log_record_sentinel_t 							sentinel;
	};
};
#endif

/* Transaction Descriptor 
 * We allocate a pool of transaction descriptors
 * Then each thread has a pointer to its transaction descriptor saved in
 * the Thread Local Storage (TLS) of each thread    
 */
struct txc_xact_descriptor_s {
	/* TRANSACTIONAL DATA */
	int xact_end_retval;
	char pad0[128];
  /* conditional synchronization variables */
  struct txc_xact_descriptor_s* waitnext; 
  struct txc_xact_descriptor_s** waitprev;
	unsigned int cond_wait_block;
	char pad1[128]; 

	/* NON-TRANSACTIONAL DATA */
	int tid;
	unsigned int stid;
	int generic_actions_registered;
	int lock_level;
	pthread_mutex_t *lock_held[10];
	int xact_level;
	int rwlock_level;
	struct rwlock_held_s {
		pthread_rwlock_t *rwlock;
		int type;
	} rwlock_held[10];
	int transaction_log_len;
	txc_transaction_log_record_t *transaction_log;
	int transaction_log_buffer_len;
	char transaction_log_buffer[TXC_MAX_SIZE_LOG_BUFFER];				// FIXME: log_buffer seems to be a misleading name -- use stack_buffer instead???
	txc_stat_collection_t *stat_collection;
	txc_logger_private_log_t *logger_private_log;
};


struct txc_transactionmgr_s {
	pthread_mutex_t 					mutex;
	txc_xact_descriptor_t 		**xact_descriptor_tbl;
	unsigned int 							num_threads;
	txc_koamgr_t 							*koamgr;
	txc_sentinelmgr_t 				*sentinelmgr;
};

TM_WAIVER txc_xact_descriptor_t *txc_descriptor_get();
TM_WAIVER int txc_transaction_xact_level_get();
TM_WAIVER void txc_transaction_xact_level_incr(int value);
TM_WAIVER void txc_register_commit_action(txc_result_t (*action)(unsigned int, unsigned int *), unsigned int num_args, unsigned int *arg_list);
TM_WAIVER void txc_register_compensating_action(txc_result_t (*action)(unsigned int, unsigned int *), unsigned int num_args, unsigned int *arg_list);
TM_WAIVER void txc_transaction_retry();
TM_WAIVER int txc_transaction_switch_to_irrevocable();

TM_WAIVER int txc_transaction_xact_state_get();
TM_WAIVER txc_result_t txc_transaction_log_buffer_allocate(int size, void **buffer);

TM_WAIVER void txc_transaction_self_wound();
TM_WAIVER _ITM_userCommitFunction txc_generic_commit_action(void *arg);

TM_WAIVER void txc_transaction_log_print(txc_xact_descriptor_t *xact_descriptor);


int __pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int __pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int __pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int __pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int __pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int __pthread_mutex_lock(pthread_mutex_t *mutex);
int __pthread_mutex_trylock(pthread_mutex_t *mutex);
int __pthread_mutex_unlock(pthread_mutex_t  *mutex);

extern __thread txc_xact_descriptor_t *txc_xact_descriptor;
extern txc_transactionmgr_t *txc_transactionmgr;
extern struct timeval txc_initialization_time;
extern unsigned int txc_debug_level;
extern pthread_rwlock_t *txc_irrevocable_rwl;


#endif	/* _TXC_TRANSACTION_H */
