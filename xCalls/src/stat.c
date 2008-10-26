#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <txc/int/transaction.h>
#include <txc/int/hash.h>
#include <txc/int/stat.h>
#include <txc/int/result.h>
#include <txc/int/rtconfig.h>
#include <txc/int/debug.h>
/*
 * The following structure is allocated per thread 
 *
 *                                 PROVIDER
 * +------------------------------------------------------------------------+
 * | HASH/LINEAR                                                            |
 * |    TABLE                            EVENTSET                           |
 * |   +----+     +-----------------------------------------------------+   |
 * |   |    |---->|    EVENTS                                           |   |
 * |   +----+     | +--+--+--+--+--+                                    |   |
 * |   |    |     | |  |  |  |  |  |                                    |   | 
 * |   +----+     | +--+--+--+--+--+                                    |   |
 * |   |    |     |  |     |              +-------------+               |   |
 * |   +----+     |  |     +------------->| EVENT DATA  |               |   |
 * |     .        |  |                    +-------------+               |   |
 * |     .        |  |                           .                      |   |
 * |     .        |  |                           .                      |   |
 * |   +----+     |  |                           .                      |   |
 * |   |    |     |  |                    +-------------+               |   | 
 * |   +----+     |  +------------------->| EVENT DATA  |               |   |
 * |              |                       +-------------+               |   |
 * |              | +---------------+                                   |   |
 * |              | | EVENTSET DATA |                                   |   | 
 * |              | +---------------+                                   |   |
 * |              +-----------------------------------------------------+   | 
 * |                                                                        |
 * | +-----------------+                                                    |
 * | |  PROVIDER DATA  |                                                    |
 * | +-----------------+                                                    |
 * +------------------------------------------------------------------------+
 */

static unsigned int txc_stat_mode;

struct txc_stat_event_s {
	void *data; 
	txc_stat_eventset_t *eventset; 
};

struct txc_stat_eventset_s {
	txc_stat_event_t *events;
	void *data;	
	void *key;
};
 
struct txc_stat_provider_hash_int_s {
	void (**event_probe)(txc_stat_event_t *, void *arg);
	void (**event_report)(txc_stat_event_t *);
	txc_hash_int_t *hash; 
	txc_stat_eventset_t *eventsets;
	char **event_description;
	unsigned int num_allocated_eventsets;
	unsigned int capacity;
	unsigned int numevents;
	unsigned int active_key;		/* use this if key passed to the probe is unknown */
	void *data;
}; 

struct txc_stat_provider_hash_str_s {
	void (**event_probe)(txc_stat_event_t *, void *arg);
	void (**event_report)(txc_stat_event_t *);
	txc_hash_str_t *hash; 
	txc_stat_eventset_t *eventsets;
	char **event_description;
	unsigned int num_allocated_eventsets;
	unsigned int capacity;
	unsigned int numevents;
	char * active_key;		/* use this if key passed to the probe is unknown */
	void *data;
}; 


struct txc_stat_provider_linear_s {
	void (**event_probe)(txc_stat_event_t *, void *arg);
	void (**event_report)(txc_stat_event_t *);
	txc_stat_eventset_t *eventsets; 
	char **event_description;
	unsigned int capacity;
	unsigned int numevents;
	unsigned int active_key;		/* use this if key passed to the probe is unknown */
}; 

static txc_result_t
stat_provider_hash_int_constructor(txc_stat_provider_hash_int_t **stat_providerp,
																										unsigned int capacity);
static txc_result_t
stat_provider_hash_str_constructor(txc_stat_provider_hash_str_t **stat_providerp,
																										unsigned int capacity);
static txc_result_t
stat_provider_linear_constructor(txc_stat_provider_linear_t **stat_providerp,
																										unsigned int capacity);

#define ACTION(provider, type, key_type, size, num_events) 								\
	typedef struct txc_stat_provider_##type##_s txc_stat_##provider##_t;
FOREACH_STAT_PROVIDER(ACTION)
#undef ACTION

struct txc_stat_collection_s {
	#define ACTION(provider, type, key_type, size, num_events) 							\
			struct txc_stat_provider_##type##_s *provider;
	FOREACH_STAT_PROVIDER(ACTION)
	#undef ACTION
};	

struct txc_stat_global_data_s {
	#define ACTION(provider, type, key_type, size, num_events) 							\
			struct txc_stat_##provider##_global_data_s * provider##_global_data;
	FOREACH_STAT_PROVIDER(ACTION)
	#undef ACTION
} txc_stat_global_data;	


#define PROVIDER_GLOBAL_DATA_T(provider)																	\
	txc_stat_##provider##_global_data_t

#define PROVIDER_GLOBAL_DATA(provider)																		\
	typedef struct txc_stat_##provider##_global_data_s											\
			txc_stat_##provider##_global_data_t;																\
	struct txc_stat_##provider##_global_data_s
	
#define PROVIDER_DATA_T(provider)																					\
	txc_stat_##provider##_data_t

#define PROVIDER_DATA(provider)																						\
	typedef struct txc_stat_##provider##_data_s															\
			txc_stat_##provider##_data_t;																				\
	struct txc_stat_##provider##_data_s

#define ACCESS_PROVIDER_DATA(provider)																			\
	((txc_stat_##provider##_data_t *) 																			\
			txc_xact_descriptor->stat_collection->##provider##->data)

#define ACCESS_PROVIDER_GLOBAL_DATA(provider)																\
	((txc_stat_##provider##_global_data_t *) 																\
			txc_stat_global_data.##provider##_global_data)

#define EVENT_DATA_T(provider, event)																			\
	txc_stat_##provider##_##event##_data_t

#define EVENT_DATA(provider, event)																				\
	typedef struct txc_stat_##provider##_##event##_data_s										\
			txc_stat_##provider##_##event##_data_t;															\
	struct txc_stat_##provider##_##event##_data_s

#define ACCESS_EVENT_DATA(provider, event)																	\
	((txc_stat_##provider##_##event##_data_t *) this->data)

#define ACCESS_EVENT_EVENTSET_DATA(provider)																\
	((txc_stat_##provider##_eventset_data_t *) this->eventset->data)

#define EVENTSET_DATA_T(provider)																					\
	txc_stat_##provider##_eventset_data_t

#define EVENTSET_DATA(provider)																						\
	typedef struct txc_stat_##provider##_eventset_data_s										\
			txc_stat_##provider##_eventset_data_t;															\
	struct txc_stat_##provider##_eventset_data_s

#define ACCESS_EVENTSET_DATA(provider)																			\
	((txc_stat_##provider##_eventset_data_t	*) this->data)

#define EVENT_INIT(provider, event)																				\
	void 																																		\
	txc_stat_##provider##_##event##_event_init(txc_stat_event_t *this)	

#define EVENTSET_INIT(provider)																						\
	void 																																		\
	txc_stat_##provider##_eventset_init(txc_stat_eventset_t *this)	

#define PROVIDER_INIT(provider)																						\
	void																																		\
	txc_stat_##provider##_init()

#define PROVIDER_GLOBAL_INIT(provider)																		\
	void																																		\
	txc_stat_##provider##_global_init()

#define EVENT_PROBE(provider, event)																			\
	void																																		\
	txc_stat_##provider##_##event##_probe(txc_stat_event_t *this, void *arg)

#define EVENT_REPORT(provider, event)																			\
	void																																		\
	txc_stat_##provider##_##event##_report(txc_stat_event_t *this)


#define stat_printf(arg, ...) fprintf(TXC_STAT_FPRINTFOUT, 								\
																								arg, ##__VA_ARGS__)

/* Definitions of event data, initializers and probes are defined in a 
 * separate file for ease of reading and config 
 */	
#include "stat_providers.c"

static inline
txc_result_t
stat_provider_linear_get_eventset(txc_stat_provider_linear_t *stat_providerp,
																				unsigned int key,
																				txc_stat_eventset_t **eventset)
{
	*eventset =  &(stat_providerp->eventsets[key]);
	(*eventset)->key = (void *) key;
	return TXC_R_SUCCESS;
}

#define ACTION(type, key_type)																							\
	static																																		\
	txc_result_t																															\
	stat_eventset_allocate_##type(txc_stat_provider_##type##_t *p,				  	\
																							txc_stat_eventset_t **ep) {		\
		if (txc_stat_mode == TXC_STAT_MODE_NONE) {															\
			return TXC_R_STATNOTENABLED;																					\
		}																																				\
		assert(p != NULL);																											\
		if (p->num_allocated_eventsets == p->capacity) {												\
			return TXC_R_NOMEMORY;																								\
		}																																				\
		*ep = &(p->eventsets[p->num_allocated_eventsets++]); 										\
		return TXC_R_SUCCESS; 																									\
	}																																					\
																																						\
	static inline																															\
	txc_result_t																															\
	stat_provider_##type##_get_eventset(																			\
																txc_stat_provider_##type##_t *stat_provider,\
																				key_type key,												\
																				txc_stat_eventset_t **eventset)			\
	{																																					\
		txc_result_t result;																										\
		txc_stat_eventset_t *stat_eventset;																			\
		stat_eventset = txc_##type##_get(stat_provider->hash, key);							\
		if (stat_eventset == NULL) {																						\
			/* If key not seen before then allocate a record */										\
			if ((result = 																												\
						stat_eventset_allocate_##type(stat_provider, &stat_eventset)) 	\
						!= TXC_R_SUCCESS) {																							\
				return result;																											\
			}																																			\
			if ((result = txc_##type##_add(stat_provider->hash,										\
																					 key, (void *) stat_eventset)) 		\
							!= TXC_R_SUCCESS) {																						\
				return result;																											\
			}																																			\
			stat_eventset->key = (void *) key;																		\
		}																																				\
		*eventset = stat_eventset;																							\
		return TXC_R_SUCCESS;																										\
	}

ACTION(hash_str, char * )
ACTION(hash_int, unsigned int)
#undef ACTION


/* Create wrappers for indexing the hash or linear table, and for 
 * constructing and properly initializing those tables 
 */ 

#define ACTION1(provider, event, id, descr)																\
	(*providerp)->event_description[id] = descr;														\
	(*providerp)->event_probe[id] = 																				\
			txc_stat_##provider##_##event##_probe;															\
	(*providerp)->event_report[id] =																				\
			txc_stat_##provider##_##event##_report;															\
	{																																				\
		int i;																																\
		txc_stat_##provider##_##event##_data_t *data;													\
		data = (txc_stat_##provider##_##event##_data_t *)											\
							calloc(capacity, 																						\
												sizeof(txc_stat_##provider##_##event##_data_t));	\
		for (i=0; i<capacity; i++) {																					\
			(*providerp)->eventsets[i].events[id].eventset =										\
									 (void *) &((*providerp)->eventsets[i]); 								\
			(*providerp)->eventsets[i].events[id].data = (void *) (&data[i]); 	\
			txc_stat_##provider##_##event##_event_init													\
																(&(*providerp)->eventsets[i].events[id]);	\
		}																																			\
	}

#define ACTION(provider, type, key_type, size, num_events)					 			\
	txc_result_t																														\
	txc_stat_##provider##_get_eventset(																			\
														txc_stat_provider_##type##_t *stat_provider,	\
                            provider##_key_type_t key,										\
														txc_stat_eventset_t **eventset)								\
	{																																				\
		txc_result_t result;																									\
		stat_provider_##type##_get_eventset(stat_provider, key, eventset);		\
		return result;																												\
	}																																				\
																																					\
	void 																																		\
	txc_stat_##provider##_provider_constructor(															\
														txc_stat_provider_##type##_t **providerp)			\
	{																																				\
		int capacity = size;																									\
		stat_provider_##type##_constructor(providerp, size); 									\
		(*providerp)->numevents = num_events;																	\
		(*providerp)->event_description = 																		\
					(char **)																												\
																	calloc(num_events, sizeof(char *));			\
		(*providerp)->event_probe = 																					\
					(void (**)(txc_stat_event_t *, void *))													\
																	calloc(num_events, sizeof (void (*)()));\
		(*providerp)->event_report =																					\
					(void (**)(void *, void *, unsigned int))												\
																	calloc(num_events, sizeof (void (*)()));\
		(*providerp)->data =																									\
					(txc_stat_##provider##_data_t *)																\
														malloc(sizeof (txc_stat_##provider##_data_t));\
		txc_stat_##provider##_init();																					\
		{																																			\
			int i;																															\
			txc_stat_##provider##_eventset_data_t *data;												\
			data = (txc_stat_##provider##_eventset_data_t *)										\
							calloc(capacity, 																						\
												sizeof(txc_stat_##provider##_eventset_data_t));		\
			for (i=0; i<capacity; i++) {																				\
				(*providerp)->eventsets[i].data = (void *) (&data[i]); 						\
				(*providerp)->eventsets[i].events = (txc_stat_event_t *)					\
														calloc(num_events, sizeof(txc_stat_event_t));	\
				txc_stat_##provider##_eventset_init(															\
																				&((*providerp)->eventsets[i]));		\		
			}																																		\
		}																																			\
		FOREACH_##provider##_EVENT(ACTION1)																		\
	}

FOREACH_STAT_PROVIDER(ACTION)

#undef ACTION
#undef ACTION1
#undef ACTION2


/* 
 * Create wrappers for calling probe functions, and setting provider active 
 * key indirectly 
 */

#define ACTION2(provider, event, id, descr)																	\
	void 																																			\
	txc_stat_call_##provider##_##event##_probe(provider##_key_type_t key, 		\
																																void *arg)	\
	{																																					\
		txc_stat_eventset_t *eventset;																					\
		txc_stat_event_t *event;																								\
		if (txc_stat_mode == TXC_STAT_MODE_NONE) {															\
			return;																																\
		}																																				\
		txc_stat_collection_t *stat_collection = txc_xact_descriptor->					\
																								stat_collection;						\
		if (key == TXC_STAT_KEY_UNKNOWN_##provider) {														\
			if ((key = stat_collection->##provider##->active_key) == 							\
																				TXC_STAT_KEY_UNKNOWN_##provider) {	\
				TXC_INTERNALERROR("Unknown probe key passed and active key is also	\
unknown.");																																	\
			}																																			\
		}																																				\
    txc_stat_##provider##_get_eventset(																			\
														stat_collection->##provider##, key, &eventset);	\
		event = &(eventset->events[STAT(provider, event)]);											\
		stat_collection->##provider##->																					\
												event_probe[STAT(provider, event)](event, arg);			\
	}
																																					
#define ACTION1(provider, type, key_type, size, num_events)									\
	FOREACH_##provider##_EVENT(ACTION2)																				\
	void txc_stat_##provider##_probe_key_reset()															\
	{																																					\
		if (txc_stat_mode == TXC_STAT_MODE_NONE) {															\
			return;																																\
		}																																				\
		txc_stat_collection_t *stat_collection = txc_xact_descriptor->					\
																								stat_collection;						\
		stat_collection->##provider##->active_key = 														\
																			TXC_STAT_KEY_UNKNOWN_##provider;			\
	} 																																				\
	void txc_stat_##provider##_probe_key_set(provider##_key_type_t key)				\
	{																																					\
		if (txc_stat_mode == TXC_STAT_MODE_NONE) {															\
			return;																																\
		}																																				\
		txc_stat_collection_t *stat_collection = txc_xact_descriptor->					\
																								stat_collection;						\
		stat_collection->##provider##->active_key = key;												\
	}																																					

FOREACH_STAT_PROVIDER(ACTION1)
#undef ACTION1
#undef ACTION2


txc_result_t 
txc_stat_global_init()
{
	if (strcmp(txc_runtime_settings.statistics, "disabled") == 0) {
		txc_stat_mode = TXC_STAT_MODE_NONE;
		return TXC_R_STATNOTENABLED;
	} else { 
		txc_stat_mode = TXC_STAT_MODE_SIMPLE;
	}

	/* allocate space for provider global shared data */
	#define ACTION(provider, type, key_type, size, num_events)								\
		posix_memalign(&(txc_stat_global_data.##provider##_global_data),				\
										TXC_CACHE_LINE_SIZE, 																		\
										sizeof(txc_stat_##provider##_global_data_t));

	FOREACH_STAT_PROVIDER(ACTION)
	#undef ACTION
	return TXC_R_SUCCESS;
}


txc_result_t
txc_stat_collection_create(txc_stat_collection_t **stat_collectionp) {
	if (txc_stat_mode == TXC_STAT_MODE_NONE) {
		*stat_collectionp = NULL;
		return TXC_R_STATNOTENABLED;
	}

	*stat_collectionp = (txc_stat_collection_t *) 
													malloc(sizeof(txc_stat_collection_t));
	if (*stat_collectionp == NULL) {
		return TXC_R_NOMEMORY;
	}
	#define ACTION(provider, type, key_type, size, num_events) 								\
		txc_stat_##provider##_provider_constructor															\
																	(&((*stat_collectionp)->provider));	
	FOREACH_STAT_PROVIDER(ACTION)
	#undef ACTION

	return TXC_R_SUCCESS;
}

static
txc_result_t
stat_provider_hash_int_constructor(txc_stat_provider_hash_int_t **stat_providerp,
																										unsigned int capacity)
{
	*stat_providerp = (txc_stat_provider_hash_int_t *) 
													malloc(sizeof(txc_stat_provider_hash_int_t));
	if (*stat_providerp == NULL) {
		return TXC_R_NOMEMORY;
	}

	(*stat_providerp)->eventsets = (txc_stat_eventset_t *) 
																			calloc(capacity,
																								sizeof(txc_stat_eventset_t));
	if ((*stat_providerp)->eventsets == NULL) {
		free(*stat_providerp);
		return TXC_R_NOMEMORY;
	} 
	
	if (txc_hash_int_create(&(*stat_providerp)->hash, capacity)
					== TXC_R_NOMEMORY) {
		txc_hash_int_destroy(&((*stat_providerp)->hash));
		free(*stat_providerp);	
		return TXC_R_NOMEMORY;
	} 
	(*stat_providerp)->capacity = capacity;
	(*stat_providerp)->num_allocated_eventsets = 0;
	(*stat_providerp)->active_key = 0xFFFFFFFF;
	return TXC_R_SUCCESS;
}

static
txc_result_t
stat_provider_hash_str_constructor(txc_stat_provider_hash_str_t **stat_providerp,
																										unsigned int capacity)
{
	*stat_providerp = (txc_stat_provider_hash_str_t *) 
													malloc(sizeof(txc_stat_provider_hash_str_t));
	if (*stat_providerp == NULL) {
		return TXC_R_NOMEMORY;
	}

	(*stat_providerp)->eventsets = (txc_stat_eventset_t *) 
																			calloc(capacity,
																								sizeof(txc_stat_eventset_t));
	if ((*stat_providerp)->eventsets == NULL) {
		free(*stat_providerp);
		return TXC_R_NOMEMORY;
	} 
	
	if (txc_hash_str_create(&(*stat_providerp)->hash, capacity)
					== TXC_R_NOMEMORY) {
		txc_hash_str_destroy(&((*stat_providerp)->hash));
		free(*stat_providerp);	
		return TXC_R_NOMEMORY;
	} 
	(*stat_providerp)->capacity = capacity;
	(*stat_providerp)->num_allocated_eventsets = 0;
	(*stat_providerp)->active_key = NULL;
	return TXC_R_SUCCESS;
}


static
txc_result_t
stat_provider_linear_constructor(txc_stat_provider_linear_t **stat_providerp,
																										unsigned int capacity)
{
	*stat_providerp = (txc_stat_provider_linear_t *) 
													malloc(sizeof(txc_stat_provider_linear_t));
	if (*stat_providerp == NULL) {
		return TXC_R_NOMEMORY;
	}

	(*stat_providerp)->eventsets = (txc_stat_eventset_t *) 
																			calloc(capacity,
																								sizeof(txc_stat_eventset_t));
	if ((*stat_providerp)->eventsets == NULL) {
		free(*stat_providerp);
		return TXC_R_NOMEMORY;
	} 

	(*stat_providerp)->capacity = capacity;
	return TXC_R_SUCCESS;
}


static
txc_result_t
stat_provider_print_stats(char *header, 
														void (**event_report)(txc_stat_event_t *),
														txc_stat_eventset_t *eventsets,
														char **event_description,
														unsigned int num_allocated_eventsets,
														unsigned int numevents)
{
	int i, j;
	if (num_allocated_eventsets > 0) {
		fprintf(TXC_STAT_FPRINTFOUT, "%s\n", header);
	}
	for (i=0; i<num_allocated_eventsets; i++)	{
		fprintf(TXC_STAT_FPRINTFOUT, "\tEVENTSET: 0x%x\n",
																		 (unsigned int) eventsets[i].key);
		for (j=0; j<numevents; j++) {
			event_report[j](&(eventsets[i].events[j]));	
		}
	}   
}	

static
txc_result_t
stat_provider_hash_str_print_stats(char *header,
																txc_stat_provider_hash_str_t *stat_provider)
{
	int i, j;
	if (stat_provider->num_allocated_eventsets > 0) {
		fprintf(TXC_STAT_FPRINTFOUT, "%s\n", header);
	}
	for (i=0; i<stat_provider->num_allocated_eventsets; i++)	{
		fprintf(TXC_STAT_FPRINTFOUT, "\tEVENTSET: %s\n",
																		 (char *) stat_provider->eventsets[i].key);
		for (j=0; j<stat_provider->numevents; j++) {
			stat_provider->event_report[j](&(stat_provider->eventsets[i].events[j]));	
		}
	}   
}


static
txc_result_t
stat_provider_hash_int_print_stats(char *header,
																txc_stat_provider_hash_int_t *stat_provider)
{
	return stat_provider_print_stats(header,
																			stat_provider->event_report,
																			stat_provider->eventsets,
																			stat_provider->event_description,
																			stat_provider->num_allocated_eventsets,
																			stat_provider->numevents);				
}

static
txc_result_t
stat_provider_linear_print_stats(char *header,
															txc_stat_provider_linear_t *stat_provider)
{
	return stat_provider_print_stats(header, 
																			stat_provider->event_report,
																			stat_provider->eventsets,
																			stat_provider->event_description,
																			stat_provider->capacity,
																			stat_provider->numevents);				
}


txc_result_t
txc_stat_print_stats() 
{
	int i;
	txc_result_t result;
	unsigned int num_threads;
	txc_xact_descriptor_t *xact_descriptor;
	txc_stat_collection_t *stat_collection;	

	if (txc_stat_mode == TXC_STAT_MODE_NONE) {
		return TXC_R_STATNOTENABLED;
	}
	num_threads = txc_transactionmgr->num_threads;
	for (i=0; i<num_threads; i++)	{
		xact_descriptor = txc_transactionmgr->xact_descriptor_tbl[i];
		stat_collection = xact_descriptor->stat_collection;
		fprintf(TXC_STAT_FPRINTFOUT, "Thread %d\n", i);
		#define ACTION(provider, type, key_type, size, num_events) 							\
	 		stat_provider_##type##_print_stats(#provider, 												\
																						stat_collection->##provider);	
		FOREACH_STAT_PROVIDER(ACTION)
		#undef ACTION
	}
}	
