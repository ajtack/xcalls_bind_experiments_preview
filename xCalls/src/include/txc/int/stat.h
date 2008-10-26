#ifndef _TXC_STAT_H
#define _TXC_STAT_H

#include <txc/int/result.h> 
#include <txc/int/hrtime.h> 

#define TXC_STAT_FPRINTFOUT						stdout

#define TXC_STAT_MODE_NONE						0
#define TXC_STAT_MODE_SIMPLE					1
#define TXC_STAT_MODE_VERBOSE					2


#define FOREACH_LOCKSTAT_EVENT(ACTION)																		\
	ACTION(LOCKSTAT, SPINWAIT, 	0, "Spin wait")															\
	ACTION(LOCKSTAT, BLOCKWAIT, 1, "Block wait")														\
	ACTION(LOCKSTAT, ACQUIRE, 	2, "Acquire lock")													\
	ACTION(LOCKSTAT, RELEASE, 	3, "Release lock")	


#define FOREACH_XACTSTAT_EVENT(ACTION)																		\
	ACTION(XACTSTAT, XCALL    ,	0, "xCall")																	\
	ACTION(XACTSTAT, WOULDWAIT, 1, "Would wait for glock")									\
	ACTION(XACTSTAT, BEGIN    , 2, "Begin outer transaction")								\
	ACTION(XACTSTAT, END      , 3, "End outer transaction")


#define FOREACH_SYNCSTAT_EVENT(ACTION)																		\
	ACTION(SYNCSTAT, ACQUIRE_GLOCK,	0, "Acquire Glock")											\
	ACTION(SYNCSTAT, RELEASE_GLOCK, 1, "Release Glock")


#define FOREACH_STAT_PROVIDER(ACTION)																			\
	ACTION(LOCKSTAT, hash_int, unsigned int, 1024, 4)												\
	ACTION(XACTSTAT, hash_str, char *      , 1024, 4)												\
	ACTION(SYNCSTAT, hash_str, char *      , 128, 2)		


#define TXC_STAT_KEY_UNKNOWN_LOCKSTAT					0xFFFFFFFF
#define TXC_STAT_KEY_UNKNOWN_XACTSTAT					NULL
#define TXC_STAT_KEY_UNKNOWN_SYNCSTAT					NULL

typedef struct txc_stat_event_s txc_stat_event_t;
typedef struct txc_stat_eventset_s txc_stat_eventset_t;
typedef struct txc_stat_provider_hash_int_s txc_stat_provider_hash_int_t;
typedef struct txc_stat_provider_hash_str_s txc_stat_provider_hash_str_t;
typedef struct txc_stat_provider_linear_s txc_stat_provider_linear_t;
typedef struct txc_stat_collection_s txc_stat_collection_t;

/* Private interface functions */
txc_result_t
txc_stat_collection_create(txc_stat_collection_t **stat_collectionp);
txc_result_t txc_stat_global_init();
txc_result_t txc_stat_print_stats();
 
/* Public interface functions */
#ifdef TXC_STATISTICS_ENABLED 
#define TXC_STAT_PROBE_KEY_RESET(provider)																\
 	txc_stat_##provider##_probe_key_reset();
#define TXC_STAT_PROBE_KEY_SET(provider, key)															\
 	txc_stat_##provider##_probe_key_set(key);
#define TXC_STAT_CALL_EVENT_PROBE(provider, event, key, arg)							\
	txc_stat_call_##provider##_##event##_probe(key, arg);	

#else  /* TXC_STATISTICS_ENABLED */

#define TXC_STAT_PROBE_KEY_RESET(provider) (void)0 	;
#define TXC_STAT_PROBE_KEY_SET(provider, key) (void)0	;
#define TXC_STAT_CALL_EVENT_PROBE(provider, event, key, arg)	(void)0;

#endif /* TXC_STATISTICS_ENABLED */


#define ACTION2(provider, event, id, descr)																\
	TM_WAIVER void 																													\
	txc_stat_call_##provider##_##event##_probe(provider##_key_type_t, void *);	

#define ACTION1(provider, type, key_type, size, num_events)								\
	typedef key_type provider##_key_type_t;																	\
	FOREACH_##provider##_EVENT(ACTION2)																			\
	TM_WAIVER void txc_stat_##provider##_probe_key_reset();									\
	TM_WAIVER void txc_stat_##provider##_probe_key_set(key_type);					

FOREACH_STAT_PROVIDER(ACTION1)
#undef ACTION1
#undef ACTION2


#define ACTION2(provider, event, id, descr) provider##event##Stat,
#define STAT(provider, event) provider##event##Stat

#define ACTION1(provider, type, key_type, size, num_events)								\
enum {																																		\
  FOREACH_##provider##_EVENT(ACTION2)																			\
};

FOREACH_STAT_PROVIDER(ACTION1)
#undef ACTION1
#undef ACTION2

#endif
