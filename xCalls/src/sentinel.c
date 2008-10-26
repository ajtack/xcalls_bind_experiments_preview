#define _TXC_TRANSACTION_H_INCLUDE_PRIVATE
#include <txc/int/transaction.h>
#undef _TXC_TRANSACTION_H_INCLUDE_PRIVATE
#include <txc/int/debug.h>

#define TXC_SENTINEL_DEBUG_LEVEL 3

/*
 * For each transaction we keep a list of sentinels been acquired
 * Each sentinel is marked whether it should be dropped (released) on commit
 * and abort of the transaction.
 * Sentinels which are dropped on abort are sorted and acquired in order
 * before the transaction restarts
 * INVARIANT 1: Whenever acquiring a sentinel inside a transaction we enlist
 * it and mark the sentinel to be dropped on commit/abort
 */

TM_WAIVER
txc_result_t
txc_sentinelmgr_create(txc_sentinelmgr_t **sentinelmgrp) {
	pool_element_sentinel_t *pool;
	int i;

	*sentinelmgrp = (txc_sentinelmgr_t *) malloc(sizeof(txc_sentinelmgr_t));
	if (*sentinelmgrp == NULL) {
		return TXC_R_NOMEMORY;
	}
	pool = (pool_element_sentinel_t *) calloc(TXC_SENTINEL_POOL_SIZE, 
																			sizeof(pool_element_sentinel_t));
	if (pool == NULL) {
		free(sentinelmgrp);
		return TXC_R_NOMEMORY;
	}
	for (i=0;i<TXC_SENTINEL_POOL_SIZE;i++) {
		pool[i].next = &pool[(i+1)%TXC_SENTINEL_POOL_SIZE];
		pool[i].prev = &pool[(i-1)%TXC_SENTINEL_POOL_SIZE];
		pool[i].sentinel.id = i;
		pool[i].sentinel.manager = *sentinelmgrp;
		pool[i].sentinel.pool_element = &pool[i];
		pthread_rwlock_init(&(pool[i].sentinel.rwl), NULL);
		//FIXME: No field __rw_kind in RedHat 5.0
		//pool[i].sentinel.rwl.__rw_kind = 1;		/* give writer priority over readers */
	}
	pool[0].prev = pool[TXC_SENTINEL_POOL_SIZE-1].next = NULL;
	(*sentinelmgrp)->pool_free_size = TXC_SENTINEL_POOL_SIZE;
	(*sentinelmgrp)->pool_free = &pool[0];	
	(*sentinelmgrp)->pool_allocated_size = 0;
	(*sentinelmgrp)->pool_allocated = NULL;	
	if (pthread_mutex_init(&((*sentinelmgrp)->mutex), NULL) !=0) {
		free(pool);
		free(sentinelmgrp);
		return TXC_R_NOTINITLOCK;
	}
	return TXC_R_SUCCESS;
}

TM_WAIVER
static txc_result_t
allocate_sentinel(txc_sentinelmgr_t *sentinelmgr, txc_sentinel_t **sentinelp) {
	txc_result_t result;
	pool_element_sentinel_t *pool_element;
	int found_sentinel;

	pthread_mutex_lock(&sentinelmgr->mutex);
	if (sentinelmgr->pool_free_size == 0) {
		*sentinelp = NULL;
		result = TXC_R_NOMEMORY;
		goto unlock;
	}

	/* 
   * RACE: Sentinels in the free list may await for a release action by owner.
   * This is possible because of the small race window between returning the
   * sentinel back to the free list and releasing it. Actually for calls like
	 * close() in an irrevocable transaction, this window could be even larger. 
   * Ensure that you find both a free and released sentinel.
   */
	for (pool_element = sentinelmgr->pool_free, found_sentinel=0;
					pool_element != NULL;
					pool_element = pool_element->next) {
		if (pool_element->sentinel.rwl.__data.__nr_readers || 
				pool_element->sentinel.rwl.__data.__writer) {
			/* sentinel not available -- try the next one in the list */
			continue;
		}
		/* Great! we have a sentinel */
		found_sentinel = 1;
		break;
	}
	if (found_sentinel) {
		if (pool_element == sentinelmgr->pool_free) {
			sentinelmgr->pool_free = pool_element->next;
			if (sentinelmgr->pool_free) {
				sentinelmgr->pool_free->prev = NULL;
			}
		} else {
			assert(pool_element->prev);
			pool_element->prev->next = pool_element->next;
			if (pool_element->next) {
				pool_element->next->prev = pool_element->prev;
			}
		}
		pool_element->prev = NULL;
		pool_element->next = sentinelmgr->pool_allocated;
		if (sentinelmgr->pool_allocated) {
			sentinelmgr->pool_allocated->prev = pool_element;
		}
		sentinelmgr->pool_allocated = pool_element; 
		sentinelmgr->pool_allocated_size++;
		sentinelmgr->pool_free_size--;
	} else {
		TXC_INTERNALERROR("NO SENTINEL AVAILABLE\n");
	}

	*sentinelp = &pool_element->sentinel;
	result = TXC_R_SUCCESS;
unlock:
	pthread_mutex_unlock(&sentinelmgr->mutex);
	return result;
}

TM_WAIVER
static void
free_sentinel(txc_sentinel_t **sentinelp) {
	pool_element_sentinel_t *pool_element;

	assert(sentinelp);
	pthread_mutex_lock(&((*sentinelp)->manager->mutex));
	pool_element = (*sentinelp)->pool_element;
	if (pool_element->prev) {
		pool_element->prev->next = pool_element->next;
	} else { 
		(*sentinelp)->manager->pool_allocated = pool_element->next;
	}
	if (pool_element->next) {
		pool_element->next->prev = pool_element->prev;
	}
	pool_element->next = (*sentinelp)->manager->pool_free;
	pool_element->prev = NULL;
	if ((*sentinelp)->manager->pool_free) {
		(*sentinelp)->manager->pool_free->prev = pool_element;
	} 
	(*sentinelp)->manager->pool_free = pool_element; 
	(*sentinelp)->manager->pool_allocated_size--;
	(*sentinelp)->manager->pool_free_size++;
	pthread_mutex_unlock(&((*sentinelp)->manager->mutex));
	*sentinelp = NULL;
}

TM_WAIVER
txc_result_t
txc_sentinel_create(txc_sentinelmgr_t *sentinelmgr, txc_sentinel_t **sentinelp) {
	txc_result_t result;

	result = allocate_sentinel(sentinelmgr, sentinelp);
	if (result == TXC_R_SUCCESS) {
		(*sentinelp)->owner = -1;  
		(*sentinelp)->ref = 0;
		(*sentinelp)->enlisted = 0;
		(*sentinelp)->drop_it_on_commit = 1;
		(*sentinelp)->drop_it_on_abort = 1;
	}
	return result; 
}

TM_WAIVER
void 
txc_sentinel_destroy(txc_sentinel_t **sentinel) {
	assert(*sentinel != NULL);
	free_sentinel(sentinel);
	*sentinel = NULL;
}

TM_WAIVER
txc_result_t
txc_sentinel_attach(txc_sentinelmgr_t *sentinelmgr, txc_sentinel_t **sentinelp) {
	allocate_sentinel(sentinelmgr, sentinelp);
	(*sentinelp)->owner = -1;  
	(*sentinelp)->ref = 0; 
	(*sentinelp)->enlisted = 0;
	(*sentinelp)->drop_it_on_commit = 1;
	(*sentinelp)->drop_it_on_abort = 1;
	return TXC_R_SUCCESS;
}

TM_WAIVER
void txc_sentinel_detach(txc_sentinel_t **sentinel) {
	assert(sentinel != NULL);
	free_sentinel(sentinel);
}

TM_WAIVER
void 
txc_enlist_sentinel(txc_sentinel_t *sentinel, int acquired_status) {
	int i;

	if (sentinel->enlisted == 0) {
		i = txc_xact_descriptor->transaction_log_len++;
		txc_xact_descriptor->transaction_log[i].type = TXC_LOG_RECORD_SENTINEL; 
		txc_xact_descriptor->transaction_log[i].sentinel.ptr = sentinel; 

		if (txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head == NULL) {
			txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head =
								txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_tail = \
									&txc_xact_descriptor->transaction_log[i].sentinel;
			txc_xact_descriptor->transaction_log[i].sentinel.prev = NULL; 
			txc_xact_descriptor->transaction_log[i].sentinel.next = NULL; 
		} else {
			txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_tail->next = \
									&txc_xact_descriptor->transaction_log[i].sentinel;
			txc_xact_descriptor->transaction_log[i].sentinel.prev = \
							txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_tail;
			txc_xact_descriptor->transaction_log[i].sentinel.next = NULL; 
			txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_tail = \
										&txc_xact_descriptor->transaction_log[i].sentinel;
		}
		txc_xact_descriptor->transaction_log[i].sentinel.acquired_status = acquired_status;
		sentinel->enlisted = 1;
		sentinel->drop_it_on_abort = 1;
		sentinel->drop_it_on_commit = 1;
		txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, 
														"ENLIST SENTINEL %d", 
														sentinel->id);
	}
}

TM_WAIVER
txc_result_t 
txc_sentinel_acquire_exclusive(txc_sentinel_t *sentinel, int block, 
																	int keep_list) {
	assert(txc_xact_descriptor != NULL); 
	assert(sentinel != NULL);
	if (sentinel->owner != txc_xact_descriptor->tid) {
		if (block == 0) {
			if (__pthread_rwlock_trywrlock(&sentinel->rwl) == 0) {
				sentinel->owner = txc_xact_descriptor->tid;
				if (keep_list == 1) {
					txc_enlist_sentinel(sentinel, TXC_R_SUCCESS);
				}
			} else {
				txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, 
																"ACQUIRE SENTINEL %d: BUSY",
																sentinel->id);	
				if (keep_list == 1) {
					txc_enlist_sentinel(sentinel, TXC_R_SENTINELBUSY);
				}
				return TXC_R_SENTINELBUSY;		 
			}
		} else {
			txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, 
															"ACQUIRE SENTINEL %d: COULD BLOCK",
															sentinel->id);
			assert(__pthread_rwlock_wrlock(&sentinel->rwl) == 0);
			sentinel->owner = txc_xact_descriptor->tid;
			if (keep_list == 1) {
				txc_enlist_sentinel(sentinel, TXC_R_SUCCESS);
			}
		}
	} 
	txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, 
													"ACQUIRE SENTINEL %d: SUCCESS",
													sentinel->id);	
	return TXC_R_SUCCESS;
} 

TM_WAIVER
void 
txc_sentinel_acquire_shared(txc_sentinel_t *sentinel) {
	__pthread_rwlock_rdlock(&sentinel->rwl);
}

TM_WAIVER
static void
sentinel_list_sort(txc_xact_descriptor_t *xact_descriptor) {
	txc_transaction_log_record_sentinel_t *iterator_I;
	txc_sentinel_t *sentinel_temp;
	unsigned int swapped;

	if (xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head == NULL) {
		return;
	}

	do {
		swapped = 0;
		for (iterator_I = xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head;
				 iterator_I->next != NULL;	
				 iterator_I = iterator_I->next) {
			if (iterator_I->ptr->id > iterator_I->next->ptr->id) {
				sentinel_temp = iterator_I->ptr;
				iterator_I->ptr = iterator_I->next->ptr;
				iterator_I->next->ptr = sentinel_temp;
				swapped = 1;
			}
		}
	} while (swapped);
}

TM_WAIVER
void
txc_sentinel_list_sort() {
	sentinel_list_sort(txc_xact_descriptor);
}

TM_WAIVER
void
txc_sentinel_list_acquire_in_order() {
	txc_transaction_log_record_sentinel_t *iterator;
	txc_transaction_log_record_sentinel_t *sentinel_list_head;
	txc_transaction_log_record_sentinel_t *sentinel_list_tail;

	txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, "SENTINEL LIST: ACQUIRE IN ORDER");
	//txc_transaction_log_print(txc_xact_descriptor);
	//txc_sentinel_list_print();
	sentinel_list_head = txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head;
	txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head = NULL;
	txc_xact_descriptor->transaction_log[0].xact_frame.sentinel_list_tail = NULL;
	for (iterator = sentinel_list_head;
			 iterator != NULL;	
			 iterator = iterator->next) {
		if (iterator->ptr->drop_it_on_abort == 1) {
			txc_sentinel_acquire_exclusive(iterator->ptr, 1, 1);
			//txc_transaction_log_print(txc_xact_descriptor);
			//txc_sentinel_list_print();
		}
	}
	//txc_transaction_log_print(txc_xact_descriptor);
}

TM_WAIVER
void 
txc_sentinel_list_release(int commit_or_abort) {
	int i;

	/* 
	 * COMMIT: commit_or_abort = 1
	 * ABORT : commit_or_abort = 2
	 */

	txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, "RELEASE SENTINELS");

	for (i=0; i<txc_xact_descriptor->transaction_log_len; i++) {
		if (txc_xact_descriptor->transaction_log[i].type == TXC_LOG_RECORD_SENTINEL) {
			/* Release only the sentinels we acquired and which are marked for 
			 * release
			 * If Aborted then there is one sentinel in the list that we couldn't 
			 * acquire (thus the abort) so we must NOT release it  
			 */
			if (txc_xact_descriptor->transaction_log[i].sentinel.ptr->owner 
																								== txc_xact_descriptor->tid
					&& ((commit_or_abort == 1 && 
								txc_xact_descriptor->transaction_log[i].sentinel.ptr->drop_it_on_commit == 1) 
							|| (commit_or_abort == 2 && 
								txc_xact_descriptor->transaction_log[i].sentinel.ptr->drop_it_on_abort == 1)))				{
				txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, 
																"RELEASE SENTINEL %d", 
																txc_xact_descriptor->transaction_log[i].sentinel.ptr->id);
				txc_xact_descriptor->transaction_log[i].sentinel.ptr->owner = -1;
				txc_xact_descriptor->transaction_log[i].sentinel.ptr->enlisted = 0;
				assert(__pthread_rwlock_unlock(&txc_xact_descriptor->transaction_log[i].sentinel.ptr->rwl)==0);
			}
			else {
				txc_xact_descriptor->transaction_log[i].sentinel.ptr->enlisted = 0;
				txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, 
																"NO RELEASE SENTINEL %d",
										txc_xact_descriptor->transaction_log[i].sentinel.ptr->id);
			}
		}
	}
}

TM_WAIVER
void 
txc_sentinel_release(txc_sentinel_t *sentinel) {
	sentinel->owner = -1;
	__pthread_rwlock_unlock(&sentinel->rwl);
}

TM_WAIVER
void
txc_sentinel_drop_set(txc_sentinel_t *sentinel, int on_commit, int on_abort) {
	sentinel->drop_it_on_commit = on_commit;	
	sentinel->drop_it_on_abort = on_abort;	
}

/*
 * The following are for debugging pursposes 
 */

TM_WAIVER
void 
txc_sentinelmgr_print_pools(txc_sentinelmgr_t *sentinelmgr) {
	pool_element_sentinel_t *pool_element;

	if (txc_debug_level >= TXC_SENTINEL_DEBUG_LEVEL) {
		fprintf(TXC_FPRINTFOUT, "FREE POOL\n");
		pool_element = sentinelmgr->pool_free;
		while (pool_element) {
			fprintf(TXC_FPRINTFOUT, "Sentinel ID = %d\t", pool_element->sentinel.id);
			if (pool_element->prev) {
				fprintf(TXC_FPRINTFOUT, "[prev = %d, ", pool_element->prev->sentinel.id);
			} else {
				fprintf(TXC_FPRINTFOUT, "[prev = NULL, ");
			} 
			if (pool_element->next) {
				fprintf(TXC_FPRINTFOUT, "next = %d]\n", pool_element->next->sentinel.id);
			} else {
				fprintf(TXC_FPRINTFOUT, "next = NULL]\n");
			} 
			pool_element = pool_element->next;
		}
		fprintf(TXC_FPRINTFOUT, "\nALLOCATED POOL\n");
		pool_element = sentinelmgr->pool_allocated;
		while (pool_element) {
			fprintf(TXC_FPRINTFOUT, "Sentinel ID = %d\t", pool_element->sentinel.id);
			if (pool_element->prev) {
				fprintf(TXC_FPRINTFOUT, "[prev = %d, ", pool_element->prev->sentinel.id);
			} else {
				fprintf(TXC_FPRINTFOUT, "[prev = NULL, ");
			} 
			if (pool_element->next) {
				fprintf(TXC_FPRINTFOUT, "next = %d]\n", pool_element->next->sentinel.id);
			} else {
				fprintf(TXC_FPRINTFOUT, "next = NULL]\n");
			} 
			pool_element = pool_element->next;
		}
	}
}

TM_WAIVER
void 
sentinel_list_print(txc_xact_descriptor_t *xact_descriptor) {
	txc_transaction_log_record_sentinel_t *iterator;

	txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, "SENTINEL LIST");
	for (iterator = xact_descriptor->transaction_log[0].xact_frame.sentinel_list_head;
			 iterator != NULL;	
			 iterator = iterator->next) {
		txc_debug_printmsg_L(TXC_SENTINEL_DEBUG_LEVEL, "SENTINEL %d (prev = %d, next = %d)", 
														iterator->ptr->id, 
														iterator->prev ? iterator->prev->ptr->id : -1,
														iterator->next ? iterator->next->ptr->id : -1);
	}
}

TM_WAIVER
void 
txc_sentinel_list_print() {
	sentinel_list_print(txc_xact_descriptor);
}
