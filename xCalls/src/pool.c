#include <txc/int/transaction.h>

#define TXC_POOL_DEBUG_LEVEL 3

TM_WAIVER
txc_result_t
txc_pool_create(txc_pool_t **poolp, 
										unsigned int obj_size, 
										unsigned int obj_num, 
										txc_pool_object_constructor_t obj_constructor) {
	txc_pool_object_t *object_list;
	char *buf; 
	int i;

	*poolp = (txc_pool_t *) malloc(sizeof(txc_pool_t));
	if (*poolp == NULL) {
		return TXC_R_NOMEMORY;
	}
	(*poolp)->obj_size = obj_size;
	(*poolp)->obj_num = obj_num;
	object_list = (txc_pool_object_t *) calloc(obj_num, 
																					sizeof(txc_pool_object_t));
	if (object_list == NULL) {
		free(*poolp);
		return TXC_R_NOMEMORY;
	}
	(*poolp)->obj_list = object_list;
	if ((buf = calloc(obj_num, obj_size)) == NULL) {  
		free(object_list);
		free(*poolp);
		return TXC_R_NOMEMORY;
	}
	(*poolp)->full_buf = buf; 
	for (i=0;i<obj_num;i++) {
		object_list[i].buf = &buf[i*obj_size];
		object_list[i].next = &object_list[(i+1)%obj_num];
		object_list[i].prev = &object_list[(i-1)%obj_num];
		object_list[i].status = TXC_POOL_OBJECT_FREE;
		if (obj_constructor != NULL) {
			obj_constructor(object_list[i].buf);
		}
	}
	object_list[0].prev = object_list[obj_num-1].next = NULL;
	(*poolp)->obj_free_num = obj_num;
	(*poolp)->obj_free = &object_list[0];	
	(*poolp)->obj_allocated_num = 0;
	(*poolp)->obj_allocated = NULL;	
	if (pthread_mutex_init(&((*poolp)->mutex), NULL) !=0) {
		free(buf);
		free(object_list);	
		free(*poolp);
		return TXC_R_NOTINITLOCK;
	}
	return TXC_R_SUCCESS;
}

TM_WAIVER
void 
txc_pool_destroy(txc_pool_t **poolp) {
	assert(*poolp != NULL);
	free((*poolp)->full_buf);
	free((*poolp)->obj_list);	
	free(*poolp);
	*poolp = NULL;
}

TM_WAIVER
txc_result_t
txc_pool_object_alloc(txc_pool_t *pool, void **objp) {
	txc_result_t result;
	txc_pool_object_t *pool_object;

	pthread_mutex_lock(&pool->mutex);
	if (pool->obj_free_num == 0) {
		*objp = NULL;
		result = TXC_R_NOMEMORY;
		goto unlock;
	}
	pool_object = pool->obj_free;
	pool->obj_free = pool_object->next;
	if (pool->obj_free) {
		pool->obj_free->prev = NULL;
	}
	pool_object->prev = NULL;
	pool_object->next = pool->obj_allocated;
	if (pool->obj_allocated) {
		pool->obj_allocated->prev = pool_object;
	}
	pool->obj_allocated = pool_object; 
	pool->obj_allocated_num++;
	pool->obj_free_num--;
	*objp = pool_object->buf;
	pool_object->status = TXC_POOL_OBJECT_ALLOCATED;
	result = TXC_R_SUCCESS;
unlock:
	pthread_mutex_unlock(&pool->mutex);
	return result;
}

TM_WAIVER
void
txc_pool_object_free(txc_pool_t *pool, void **objp) {
	txc_pool_object_t *pool_object;
	unsigned int index;

	assert(pool);
	assert(*objp);
	pthread_mutex_lock(&(pool->mutex));
	index = ((unsigned int) (*objp - pool->full_buf)) / pool->obj_size;
	pool_object = &pool->obj_list[index];
	assert(pool_object->status == TXC_POOL_OBJECT_ALLOCATED);
	if (pool_object->prev) {
		pool_object->prev->next = pool_object->next;
	} else { 
		pool->obj_allocated = pool_object->next;
	}
	if (pool_object->next) {
		pool_object->next->prev = pool_object->prev;
	}
	pool_object->next = pool->obj_free;
	pool_object->prev = NULL;
	if (pool->obj_free) {
		pool->obj_free->prev = pool_object;
	} 
	pool->obj_allocated_num--;
	pool->obj_free_num++;
	pool_object->status = TXC_POOL_OBJECT_FREE;
	pool->obj_free = pool_object; 
	pthread_mutex_unlock(&(pool->mutex));
	*objp = NULL;
}

void 
txc_pool_print(txc_pool_t *pool, txc_pool_object_print_t printer) {
	txc_pool_object_t *pool_object;
	unsigned int index;

	if (txc_debug_level >= TXC_POOL_DEBUG_LEVEL) {
		fprintf(TXC_FPRINTFOUT, "FREE POOL\n");
		pool_object = pool->obj_free;
		while (pool_object) {
			index = (pool_object - pool->obj_list);
			fprintf(TXC_FPRINTFOUT, "Object Index = %u\t", index);
			if (pool_object->prev) {
				index = (pool_object->prev - pool->obj_list);
				fprintf(TXC_FPRINTFOUT, "[prev = %u, ", index);
			} else {
				fprintf(TXC_FPRINTFOUT, "[prev = NULL, ");
			} 
			if (pool_object->next) {
				index = (pool_object->next - pool->obj_list);
				fprintf(TXC_FPRINTFOUT, "next = %u]\n", index);
			} else {
				fprintf(TXC_FPRINTFOUT, "next = NULL]\n");
			}
			if (printer) {
				printer(pool_object->buf);
			} 
			pool_object = pool_object->next;
		}
		fprintf(TXC_FPRINTFOUT, "\nALLOCATED POOL\n");
		pool_object = pool->obj_allocated;
		while (pool_object) {
			index = (pool_object - pool->obj_list);
			fprintf(TXC_FPRINTFOUT, "Object Index = %u\t", index);
			if (pool_object->prev) {
				index = (pool_object->prev - pool->obj_list);
				fprintf(TXC_FPRINTFOUT, "[prev = %u, ", index);
			} else {
				fprintf(TXC_FPRINTFOUT, "[prev = NULL, ");
			} 
			if (pool_object->next) {
				index = (pool_object->next - pool->obj_list);
				fprintf(TXC_FPRINTFOUT, "next = %u]\n", index);
			} else {
				fprintf(TXC_FPRINTFOUT, "next = NULL]\n");
			} 
			if (printer) {
				printer(pool_object->buf);
			} 
			pool_object = pool_object->next;
		}
	}
}
