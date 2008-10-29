#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include <sched.h>
#include <ia32intrin.h>
#define _TXC_TRANSACTION_H_INCLUDE_PRIVATE
#include <txc/int/transaction.h>
#undef _TXC_TRANSACTION_H_INCLUDE_PRIVATE
#include <txc/int/debug.h>


unsigned int txc_debug_level;

TM_WAIVER _ITM_userCommitFunction txc_generic_commit_action();
TM_WAIVER _ITM_userUndoFunction txc_generic_compensating_action();

__thread txc_xact_descriptor_t *txc_xact_descriptor;	// Thread Local Storage
txc_transactionmgr_t *txc_transactionmgr;
struct timeval txc_initialization_time;
pthread_t contentionmgr;


TM_WAIVER
txc_xact_descriptor_t *
txc_descriptor_get() 
{
	return txc_xact_descriptor;
}


TM_WAIVER
int
txc_transaction_xact_state_get() 
{
	_ITM_transaction * td;
	_ITM_howExecuting mode;

  if (txc_transactionmgr) {
		td = _ITM_getTransaction();
		mode = _ITM_inTransaction(td);
		switch (mode) {
			case outsideTransaction:
				return TXC_XACT_STATE_NON_TRANSACTIONAL;
			case inRetryableTransaction:
				return TXC_XACT_STATE_RUNNING_SPECULATIVE;
			case inIrrevocableTransaction:
				return TXC_XACT_STATE_RUNNING_IRREVOCABLE;
			default:
				assert(0);
		}
  } else {
    return TXC_XACT_STATE_NOT_INIT;
  }
}


TM_WAIVER
int 
txc_transaction_switch_to_irrevocable() 
{
}


txc_result_t
txc_transactionmgr_create(txc_transactionmgr_t **transactionmgrp) 
{
	txc_result_t result; 
	txc_koa_t *koa;
	int i;

	*transactionmgrp = (txc_transactionmgr_t *) malloc(sizeof(
																							txc_transactionmgr_t));
	if (*transactionmgrp == NULL) {
		return TXC_R_NOMEMORY;
	}
	//FIXME: No field __rw_kind in RedHat 5.0
  //txc_irrevocable_rwl->__rw_kind = 1;   /* give writer priority over readers */
	(*transactionmgrp)->xact_descriptor_tbl = (txc_xact_descriptor_t**) \
									calloc(TXC_MAX_NUM_THREADS, sizeof(txc_xact_descriptor_t*));
	if ((*transactionmgrp)->xact_descriptor_tbl == NULL) {
		free(*transactionmgrp);
		return TXC_R_NOMEMORY;
	}
	for (i = 0; i < TXC_MAX_NUM_THREADS; ++i) {
		(*transactionmgrp)->xact_descriptor_tbl[i] = NULL;
	}
	if (pthread_mutex_init(&((*transactionmgrp)->mutex), NULL) != 0) { 
		free((*transactionmgrp)->xact_descriptor_tbl);
		free(*transactionmgrp);
		return TXC_R_NOTINITLOCK;
	}
	if ((result = 
				txc_koamgr_create(&((*transactionmgrp)->koamgr))) != TXC_R_SUCCESS)	{
		free((*transactionmgrp)->xact_descriptor_tbl);
		free(*transactionmgrp);
		return result;
	}
	if ((result = 
				txc_sentinelmgr_create(&((*transactionmgrp)->sentinelmgr)))
																													 != TXC_R_SUCCESS) {
		txc_koamgr_destroy(&((*transactionmgrp)->koamgr));
		free((*transactionmgrp)->xact_descriptor_tbl);
		free(*transactionmgrp);
		return result;
	}
	/* Assign sentinels to the standard input, output, error */
	txc_koa_create((*transactionmgrp)->koamgr, &koa, TXC_KOA_IS_FILE);
	txc_koa_attach(txc_transactionmgr->koamgr, koa, 0);
	txc_koa_create((*transactionmgrp)->koamgr, &koa, TXC_KOA_IS_FILE);
	txc_koa_attach(txc_transactionmgr->koamgr, koa, 1);
	txc_koa_create((*transactionmgrp)->koamgr, &koa, TXC_KOA_IS_FILE);
	txc_koa_attach(txc_transactionmgr->koamgr, koa, 2);
	(*transactionmgrp)->num_threads = 0;
	if (getenv("TXC_DEBUG_LEVEL") != NULL) {
		txc_debug_level = atoi(getenv("TXC_DEBUG_LEVEL"));
	} else {
		txc_debug_level = 0;
	}
	txc_logger_global_create(); 
	return TXC_R_SUCCESS;
}

txc_result_t 
txc_xact_descriptor_create(txc_transactionmgr_t *transactionmgr, 
															txc_xact_descriptor_t **xact_descriptorp) {
	int tid, i;
	txc_result_t;
	unsigned long mask;

	assert(transactionmgr != NULL);
	posix_memalign((void **) xact_descriptorp, 512, 
																		sizeof(txc_xact_descriptor_t));
	if (*xact_descriptorp == NULL) {
		return TXC_R_NOMEMORY;
	}
	pthread_mutex_lock(&(transactionmgr->mutex));
	tid = (*xact_descriptorp)->tid = transactionmgr->num_threads++;
	pthread_mutex_unlock(&(transactionmgr->mutex));
	(*xact_descriptorp)->xact_end_retval = 0;
	(*xact_descriptorp)->stid = pthread_self();
	(*xact_descriptorp)->lock_level = 0;
	(*xact_descriptorp)->rwlock_level = 0;
	for (i=0; i<10; i++) {	
		(*xact_descriptorp)->lock_held[i] = NULL;
		(*xact_descriptorp)->rwlock_held[i].rwlock = NULL;
		(*xact_descriptorp)->rwlock_held[i].type = 0;
	}
	(*xact_descriptorp)->xact_level = 0;

	/* Init Log */
	(*xact_descriptorp)->transaction_log = (txc_transaction_log_record_t *)
			calloc(TXC_MAX_NUM_LOG_RECORDS, sizeof(txc_transaction_log_record_t));
	(*xact_descriptorp)->transaction_log_len = 1;
	(*xact_descriptorp)->transaction_log[0].type = TXC_LOG_RECORD_FRAME;
	(*xact_descriptorp)->transaction_log[0].xact_frame.sentinel_list_head =
																																			 NULL;
	(*xact_descriptorp)->transaction_log[0].xact_frame.sentinel_list_tail =
																																			 NULL;
	(*xact_descriptorp)->transaction_log[0].xact_frame.has_commit_actions = 0;
	(*xact_descriptorp)->transaction_log[0].xact_frame.has_compensating_actions = 0;

	(*xact_descriptorp)->transaction_log_buffer_len = 0;
	(*xact_descriptorp)->generic_actions_registered = 0;

	/* Init statistics */
	if (txc_stat_collection_create(&(*xact_descriptorp)->stat_collection)
					== TXC_R_NOMEMORY) {
		return TXC_R_NOMEMORY;
	}
	if (txc_logger_private_create(&(*xact_descriptorp)->logger_private_log, tid)
					== TXC_R_NOMEMORY) {
		return TXC_R_NOMEMORY;
	}	
	transactionmgr->xact_descriptor_tbl[tid] = *xact_descriptorp;
//	FIXME: Make CPU affinity an option
//	mask = 0b01111111;
//	assert(sched_setaffinity(0, sizeof(mask), &mask) == 0); 
	return TXC_R_SUCCESS;
}	

static 
void txc_termination() 
{
	txc_logger_terminate();
}

void txc_global_init() 
{
	txc_result_t result;

	txc_rtconfig_runtime_settings_init();
	result = txc_transactionmgr_create(&txc_transactionmgr);
	txc_assert(result == TXC_R_SUCCESS);	
	txc_stat_global_init();
	atexit(txc_termination);
	gettimeofday(&txc_initialization_time, NULL);
}

void txc_thread_init() 
{
	txc_result_t result;

	result = 
				txc_xact_descriptor_create(txc_transactionmgr, &txc_xact_descriptor);
	assert(result == TXC_R_SUCCESS);	
}


static inline
void 
register_generic_actions(txc_xact_descriptor_t *desc)
{
	if (desc->generic_actions_registered == 0) {
		/* FIXME: When transitioning to irrevocable mode, the TM system executes 
		 * any registered commit actions. As a result the TM system will execute
		 * the generic commit action. This has a lot of problems, e.g. don't
		 * release sentinels at commit, etc.
		 * For now we execute commit actions by glueing code at the end of the
		 * atomic construct but this has the disadvantage that the exit point
		 * must be the end of the transaction (no gotos, no returns, etc)    
		 * A better solution is to use the Obstinate mode instead of the 
		 * irrevocable mode or modify the glock mode in the STM (sources???)  
		 *
		 *
		 *	_ITM_addUserCommitAction(_ITM_getTransaction(), 
		 *																	txc_generic_commit_action, 1, NULL);	
		 */

		_ITM_addUserUndoAction(_ITM_getTransaction(),
																		txc_generic_compensating_action, NULL);
		desc->generic_actions_registered = 1;
	}
}

TM_WAIVER
void
txc_transaction_retry()
{
	register_generic_actions(txc_xact_descriptor);
	_ITM_abortTransaction(_ITM_getTransaction(), userRetry, NULL);
}


TM_WAIVER
void
txc_register_commit_action(
				txc_result_t (*action)(unsigned int, unsigned int *), 
				unsigned int num_args, 
				unsigned int *arg_list) {
	int j, index;

	if (_ITM_inTransaction(_ITM_getTransaction()) > 0) {
		assert(txc_xact_descriptor != NULL);
		register_generic_actions(txc_xact_descriptor);
		if (txc_xact_descriptor->transaction_log[0].xact_frame.has_commit_actions == 0) {
			txc_xact_descriptor->transaction_log[0].xact_frame.has_commit_actions = 1; 
		} 
		index = txc_xact_descriptor->transaction_log_len++;
		txc_xact_descriptor->transaction_log[index].type = TXC_LOG_RECORD_COMMIT_ACTION;
		txc_xact_descriptor->transaction_log[index].commit_action.action = action;
		txc_xact_descriptor->transaction_log[index].commit_action.num_args = num_args;
		for (j=0; j<num_args; j++) {
			txc_xact_descriptor->transaction_log[index].commit_action.arg_list[j] = arg_list[j];
		}
	} else {
		action(num_args, arg_list);
	}
}

TM_WAIVER
void 
txc_register_compensating_action(
				txc_result_t (*action)(unsigned int, unsigned int *), 
				unsigned int num_args, 
				unsigned int *arg_list) {
	int j, index;

	if (_ITM_inTransaction(_ITM_getTransaction()) > 0) {
		assert(txc_xact_descriptor != NULL);
		register_generic_actions(txc_xact_descriptor);
		if (txc_xact_descriptor->
							transaction_log[0].xact_frame.has_compensating_actions == 0) {
			txc_xact_descriptor->
							transaction_log[0].xact_frame.has_compensating_actions = 1; 
		} 
		index = txc_xact_descriptor->transaction_log_len++;
		txc_xact_descriptor->transaction_log[index].type = TXC_LOG_RECORD_COMPENSATING_ACTION;
		txc_xact_descriptor->transaction_log[index].compensating_action.action = action;
		txc_xact_descriptor->transaction_log[index].compensating_action.num_args = num_args;
		for (j=0; j<num_args; j++) {
			txc_xact_descriptor->transaction_log[index].compensating_action.arg_list[j] = arg_list[j];
		}
	}
}


static inline
void 
transaction_reset(txc_xact_descriptor_t *desc)
{
	/* Reset Log */
	desc->transaction_log_len = 1;
	desc->transaction_log[0].xact_frame.sentinel_list_head = NULL;
	desc->transaction_log[0].xact_frame.sentinel_list_tail = NULL;
	desc->transaction_log[0].xact_frame.has_commit_actions = 1; 
	desc->transaction_log[0].xact_frame.has_compensating_actions = 1; 

	desc->transaction_log_buffer_len = 0;
	desc->generic_actions_registered = 0;
}

TM_WAIVER
_ITM_userCommitFunction
txc_generic_commit_action(void *arg) {
	int i;
	txc_result_t result;

	/* Execute TXC commit actions */
	if (txc_xact_descriptor->transaction_log[0].xact_frame.has_commit_actions) { 
		for (i=1; i<txc_xact_descriptor->transaction_log_len; i++) {
			if (txc_xact_descriptor->transaction_log[i].type == TXC_LOG_RECORD_COMMIT_ACTION) {
				result = txc_xact_descriptor->transaction_log[i].commit_action.action(
														txc_xact_descriptor->transaction_log[i].commit_action.num_args,
														txc_xact_descriptor->transaction_log[i].commit_action.arg_list);
			}
		} 
	}

	/* Release sentinels and then drop sentinel list */
	txc_sentinel_list_release(1);

	txc_debug_printmsg_L(2, "GENERIC COMMIT ACTION: DONE");

	transaction_reset(txc_xact_descriptor);	
}

TM_WAIVER
_ITM_userUndoFunction 
txc_generic_compensating_action(void *arg) {
	int i;
	txc_result_t result;

	/* Execute TXC compensating actions */
	if (txc_xact_descriptor->transaction_log[0].xact_frame.has_compensating_actions) { 
		for (i=txc_xact_descriptor->transaction_log_len-1; i>=1 ; i--) {
			if (txc_xact_descriptor->transaction_log[i].type 
																			== TXC_LOG_RECORD_COMPENSATING_ACTION) {
					result = txc_xact_descriptor->transaction_log[i].compensating_action.action(
											txc_xact_descriptor->transaction_log[i].compensating_action.num_args, 
											txc_xact_descriptor->transaction_log[i].compensating_action.arg_list);
			} 
		}
	}

	/* Release sentinels */
	//txc_transaction_log_print(txc_xact_descriptor);
	txc_sentinel_list_release(2);
	/*
	// TODO: sort sentinels, re-acquire (possibly block)
	txc_sentinel_list_sort();

	*/

	txc_debug_printmsg_L(2, "GENERIC COMPENSATING ACTION: DONE");

	transaction_reset(txc_xact_descriptor);	
} 

TM_WAIVER
txc_result_t
txc_transaction_log_buffer_allocate(int size, void **buffer) {
	if (txc_xact_descriptor->transaction_log_buffer_len + size < 
																								TXC_MAX_SIZE_LOG_BUFFER) {
		*buffer = (void *) &(txc_xact_descriptor->
								transaction_log_buffer[txc_xact_descriptor->transaction_log_buffer_len]);
		txc_xact_descriptor->transaction_log_buffer_len += size;
		register_generic_actions(txc_xact_descriptor);
		return TXC_R_SUCCESS;  
	} else {
		printf("buffer_len = %d, size = %d\n", 
								txc_xact_descriptor->transaction_log_buffer_len, size);
		*buffer = NULL;
		return TXC_R_NOMEMORY;
	}
}

TM_WAIVER
void 
txc_transaction_self_wound() {
}

/*
 * Debugging functions
 */


TM_WAIVER
void 
txc_transaction_log_print(txc_xact_descriptor_t *xact_descriptor) {
	int i;

	txc_debug_printmsg_L(3, "LOG");	
	for (i=0; i < xact_descriptor->transaction_log_len; i++) {
				fprintf(TXC_FPRINTFOUT, 
									"TXC_DEBUG: Thread %d (%u) [xact_level = %d, PC = %p]\tLOG ENTRY[%d]: ",
									xact_descriptor->tid, 
									xact_descriptor->stid, 
									xact_descriptor->xact_level, 
									__builtin_return_address(0),
									i);
				switch (xact_descriptor->transaction_log[i].type) {
					case TXC_LOG_RECORD_FRAME:
						fprintf(TXC_FPRINTFOUT, "XACT FRAME\n");
						break;
					case TXC_LOG_RECORD_COMMIT_ACTION:
						fprintf(TXC_FPRINTFOUT, "COMMIT ACTION\n");
						break;
					case TXC_LOG_RECORD_COMPENSATING_ACTION:
						fprintf(TXC_FPRINTFOUT, "COMPENSATING ACTION\n");
						break;
					case TXC_LOG_RECORD_SENTINEL:
						fprintf(TXC_FPRINTFOUT, "SENTINEL %d\n",
											txc_xact_descriptor->transaction_log[i].sentinel.ptr->id);
						break;
					default:
						txc_assert(0); /* unrecognized log record type */
				}
	}
}
