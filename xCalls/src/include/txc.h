#ifndef _TXC_TXC_H
#define _TXC_TXC_H

#include <itm/itm.h>

#if (_TM >= 20071219)
	#ifndef TM_WAIVER
	#define TM_WAIVER __attribute__ ((tm_pure))
	#endif
	#ifndef TM_CALLABLE
	#define TM_CALLABLE __attribute__ ((tm_callable))
	#endif
#else
	#ifndef TM_WAIVER
	#define TM_WAIVER
	#endif
	#ifndef TM_CALLABLE
	#define TM_CALLABLE
	#endif
#endif

/* 
 * The following symbols should be defined using compiler option -D
 *  - TXC_XCALLS_ENABLE
 *  - TXC_STATISTICS_ENABLED
 */


#define TXC_ENABLE

#include <txc/int/logger.h>
#include <txc/int/transaction.h>
#include <txc/int/debug.h>

/* 
 * Transactional Macros  
 */
#ifdef TXC_ENABLE

#define XACT_END_RETVAL				 (txc_xact_descriptor->xact_end_retval) 
#define XACT_END_RETVAL_RESET  																					\
do { 																																		\
	txc_xact_descriptor->xact_end_retval = 0; 														\
} while(0);
												
#define XACT_JMP_END(tag, val)																					\
do {																																		\
	txc_xact_descriptor->xact_end_retval = val;														\
	goto tag##_end;																												\
} while(0); 

#define XACT_SAFE_ASSIGN(DEST, SRC) 																		\
										memcpy(&((DEST)), &((SRC)), sizeof((SRC)));	
#else	/* TXC_ENABLE */

#define XACT_END_RETVAL				 (0) 
#define XACT_END_RETVAL_RESET  																					
												
#define XACT_JMP_END(tag, val)																					\
do {																																		\
	goto tag##_end;																												\
} while(0); 

#define XACT_SAFE_ASSIGN(DEST, SRC) 																		\
									memcpy(&((DEST)), &((SRC)), sizeof((SRC)));	
#endif	/* TXC_ENABLE */

#define ASSERT_XACT_FUNC_PTR_INVOCATION(FUNC_PTR) do { 									\
	if (_ITM_inTransaction(_ITM_getTransaction) > 0) {										\
		assert(0);																													\
	} else {																															\
		(FUNC_PTR);																													\
	}	\
} while (0);
#define ASSERT_XACT_FUNC_INVOCATION(FUNC)																\
														ASSERT_XACT_FUNC_PTR_INVOCATION((FUNC))
#define ASSERT_NOT_RUNNING_XACT																					\
	assert(_ITM_inTransaction(_ITM_getTransaction) == 0);

#define TXC_XACT_STATE_NOT_INIT															-1
#define TXC_XACT_STATE_NON_TRANSACTIONAL										0
#define TXC_XACT_STATE_RUNNING_SPECULATIVE									1
#define TXC_XACT_STATE_RUNNING_IRREVOCABLE									2

#ifdef TXC_XCALLS_ENABLE

#define XACT_BEGIN(tag) 																										\
	if (_ITM_inTransaction(_ITM_getTransaction()) == 0) {											\
		TXC_STAT_PROBE_KEY_SET(XACTSTAT, #tag);																	\
		TXC_STAT_CALL_EVENT_PROBE(XACTSTAT, BEGIN, NULL, NULL);									\
		TXC_STAT_CALL_EVENT_PROBE(XACTSTAT, WOULDWAIT, NULL, NULL);							\
	}																																					\
	/* txc_transaction_debug("XACT BEGIN(" #tag ")", 1); */										\
/*	TXC_GLOBLOG("XACT BEGIN(" #tag ")");									*/										\
	/* FIXME: do we need to get the sentinels in irrevocable mode? */					\
	/* this should become part of the generic compensating action */					\
	/* txc_sentinel_list_acquire_in_order();	*/															\
	__tm_atomic {	


#define XACT_END(tag)																												\
	tag##_end:																																\
	}																																					\
	if (_ITM_inTransaction(_ITM_getTransaction()) == 0) {											\
		TXC_STAT_CALL_EVENT_PROBE(XACTSTAT, END, NULL, NULL)										\
		TXC_STAT_PROBE_KEY_RESET(XACTSTAT);																			\
		txc_generic_commit_action(NULL);																				\
	}	


#else	/* ! TXC_XCALLS_ENABLE */

#define XACT_BEGIN(tag) 																										\
	if (_ITM_inTransaction(_ITM_getTransaction()) == 0) {											\
		TXC_STAT_PROBE_KEY_SET(XACTSTAT, #tag);																	\
		TXC_STAT_CALL_EVENT_PROBE(XACTSTAT, BEGIN, NULL, NULL)									\
		TXC_STAT_CALL_EVENT_PROBE(XACTSTAT, WOULDWAIT, NULL, NULL)							\
	}																																					\
	__tm_atomic	{																														


#define XACT_END(tag) 																											\
	tag##_end:																																\
	}	/* __tm_atomic */																												\
	if (_ITM_inTransaction(_ITM_getTransaction()) == 0) {											\
		TXC_STAT_CALL_EVENT_PROBE(XACTSTAT, END, NULL, NULL)										\
		TXC_STAT_PROBE_KEY_RESET(XACTSTAT);																			\
		txc_generic_commit_action(NULL);																				\
	}	

#endif /* TXC_XCALLS_ENABLE */

#define XACT_BEGIN_L(tag, lock) XACT_BEGIN(tag)
#define XACT_END_L(tag, lock) XACT_END(tag)

#define	XACT_RETRY	txc_transaction_retry();

#define XACT_SWITCH_TO_IRREVOCABLE																					\
	_ITM_changeTransactionMode (_ITM_getTransaction(), 												\
																	modeSerialIrrevocable, NULL);

#define	SENTINEL_ACQUIRE_EXCLUSIVE(sentinel) 																\
	if (txc_sentinel_acquire_exclusive((sentinel), 0, 1) == 									\
																								TXC_R_SENTINELBUSY) {				\
		XACT_RETRY																															\
	}

#define	SENTINEL_ACQUIRE_SHARED(sentinel) \
	txc_sentinel_acquire_shared((sentinel));

#define	RELEASE_SENTINEL(sentinel) \
	txc_sentinel_release((sentinel));

#define	SENTINEL_RELEASE(sentinel) \
	txc_sentinel_release((sentinel));


/* Non-opaque data structures which are exposed to programs */ 
typedef struct txc_src_location_s txc_src_location_t;

struct txc_src_location_s {
	char *file;
	int line;
};   

/* Public API */
void txc_global_init();
void txc_thread_init();

#ifdef TXC_ENABLE
#include <txc/libc.h>
#include <txc/xcalls.h>
#endif

#endif
