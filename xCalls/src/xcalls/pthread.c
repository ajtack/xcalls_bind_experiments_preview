#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <txc/int/transaction.h>

#define PTHREAD_MUTEX_ROBUST_NORMAL_NP 16

TM_WAIVER
pthread_t 
txc_pthread_self(void) 
{
	return pthread_self();
}



/*
int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
	int ret;
//	printf("pthread_mutex_init(mutex = %p) CALLED BY \n", 
																			mutex, __builtin_return_address(1));
	ret = __pthread_mutex_init(mutex, attr);
	return ret;
}
*/
	
TM_WAIVER
int 
txc_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
	txc_sentinelmgr_t *sentinelmgr;
	txc_sentinel_t *sentinel;

	sentinelmgr = txc_transactionmgr->sentinelmgr;
	assert(txc_sentinel_create(sentinelmgr, &sentinel) == TXC_R_SUCCESS);
	mutex->__data.__lock = (unsigned int) sentinel;
	return 0;
}

TM_CALLABLE
int
txc_pthread_mutex_lock(pthread_mutex_t *mutex) {
	txc_sentinel_t *sentinel;

	sentinel = (txc_sentinel_t *) mutex->__data.__lock; 
	if (sentinel == NULL) {
		assert(0);
	}
	switch (txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			txc_sentinel_acquire_exclusive(sentinel, 1, 0);
			return 0;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			assert(0);
			return 0;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			if (txc_sentinel_acquire_exclusive((sentinel), 0, 1) 
																					== TXC_R_SENTINELBUSY) {
				XACT_RETRY
			}
			txc_sentinel_drop_set(sentinel, 0, 1);
			break; 
		default:
			assert(0);
	}
}

TM_WAIVER
int
txc_pthread_mutex_unlock(pthread_mutex_t *mutex) {
	txc_sentinel_t *sentinel;

	sentinel = (txc_sentinel_t *) mutex->__data.__lock; 
	switch (txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			txc_sentinel_release(sentinel);
			return 0;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			assert(0);
			return 0;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			if (sentinel->enlisted == 0) { 
				txc_enlist_sentinel(sentinel, TXC_R_SUCCESS);
				 /* sentinel->drop_it_on_commit = 1; ALREADY SET by enlist_sentinel */
				sentinel->drop_it_on_abort = 0;
			} else {
				sentinel->drop_it_on_commit = 1;
			}	
			break;
		default:
			assert(0);
	}
	return 0;

}

TM_WAIVER
int 
pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
	int ret;
	ret = __pthread_rwlock_wrlock(rwlock);
	//printf("%u %d %p ACQUIRE WRITE RWLOCK: [%p] %u\n", pthread_self(), get_xact_level(), __builtin_return_address(0), rwlock); 
//	txc_xact_descriptor->rwlock_held[txc_xact_descriptor->rwlock_level].rwlock = rwlock;
//	txc_xact_descriptor->rwlock_held[txc_xact_descriptor->rwlock_level++].type = 1;
	return ret;
}

TM_WAIVER
int 
pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
	int ret;
	ret = __pthread_rwlock_rdlock(rwlock);
	//printf("%u %d %p ACQUIRE READ RWLOCK: [%p] %u\n", pthread_self(), get_xact_level(), __builtin_return_address(0), rwlock); 
//	txc_xact_descriptor->rwlock_held[txc_xact_descriptor->rwlock_level].rwlock = rwlock;
//	txc_xact_descriptor->rwlock_held[txc_xact_descriptor->rwlock_level++].type = 2;
	return ret;
}

TM_WAIVER
int 
pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
	//printf("%u %d %p RELEASE RWLOCK: [%p] %u\n", pthread_self(), get_xact_level(), __builtin_return_address(0), rwlock); 
//	txc_xact_descriptor->rwlock_held[--txc_xact_descriptor->rwlock_level].rwlock = NULL;
//	txc_xact_descriptor->rwlock_held[txc_xact_descriptor->rwlock_level].type = 0;
	return __pthread_rwlock_unlock(rwlock);
}

TM_CALLABLE
int 
txc_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t *attr) {
	//FIXME: The following breaks with RedHat 5
	//cond->__c_waiting = NULL;
}

TM_CALLABLE
int 
txc_pthread_cond_wait_enqueue(pthread_cond_t* cond) {
  txc_xact_descriptor_t *this= txc_xact_descriptor;
  txc_xact_descriptor_t **tmp;

  /* put in wait-chain */
	XACT_BEGIN(txc_pthread_cond_wait_enqueue_xact)
	//FIXME: The following breaks with RedHat 5
  //tmp=&((txc_xact_descriptor_t *) cond->__c_waiting);
  this->waitnext = NULL;
  while (*tmp) {
		tmp=&((*tmp)->waitnext);
	}
  this->waitprev=tmp;
  *tmp=this;
	XACT_END(txc_pthread_cond_wait_enqueue_xact)

	this->cond_wait_block = 1;	

  return 0;
}

/* suspend till restart signal */
TM_WAIVER
static void __thread_suspend() {
  sigset_t mask;
  sigprocmask(SIG_SETMASK, 0, &mask);
  sigdelset(&mask, SIGUSR1);
  sigsuspend(&mask);
	printf("RECEIVED\n");
}

TM_WAIVER
int 
txc_pthread_cond_wait_block(pthread_cond_t* cond) {
  txc_xact_descriptor_t *this= txc_xact_descriptor;
  txc_xact_descriptor_t **tmp;

	if (this->cond_wait_block) {
		this->cond_wait_block = 0;

	  __thread_suspend();

  	// remove from wait-chain (if not signaled)
		XACT_BEGIN(txc_pthread_cond_wait_block_xact)
  		if (this->waitnext) {
		    this->waitnext->waitprev=this->waitprev;
    		*(this->waitprev)=this->waitnext;
		  } else {
				*(this->waitprev)=0;
			}
		XACT_END(txc_pthread_cond_wait_block_xact)
	  return 1;
	} else {
		return 0;
	}
}

/* restart a thread */
TM_WAIVER
static 
void __thread_restart(txc_xact_descriptor_t *td) {
  kill(td->stid, SIGUSR1);
  sched_yield();
  sched_yield();
}

TM_CALLABLE
int 
txc_pthread_cond_signal(pthread_cond_t* cond) {
  txc_xact_descriptor_t *this= txc_xact_descriptor;
	XACT_BEGIN(txc_pthread_cond_wait_block_xact)
		//FIXME: The following breaks with RedHat 5
  	//if (cond->__c_waiting) {
		//	__thread_restart(cond->__c_waiting);
		//}
	XACT_END(txc_pthread_cond_wait_block_xact)
  return 0;
}
