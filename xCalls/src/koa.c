#include <txc/int/transaction.h>
#include <txc/int/result.h>
#include <txc/int/koa.h>
#include <txc/int/sentinel.h>
#include <txc/int/debug.h>
#include <stdlib.h>


TM_WAIVER
txc_result_t
txc_koamgr_create(txc_koamgr_t **koamgrp) {
	int i;
	txc_result_t result;

	*koamgrp = (txc_koamgr_t *) malloc(sizeof(txc_koamgr_t));
	if (*koamgrp == NULL) {
		return TXC_R_NOMEMORY;
	}
	if ((result = txc_pool_create(&((*koamgrp)->pool_koa_obj), 
															sizeof(txc_koa_t),
															TXC_KOA_TABLE_SIZE, NULL)) 
																									!= TXC_R_SUCCESS) {
		free(*koamgrp);
		return result;
	}	
	for (i=0; i<TXC_KOA_TABLE_SIZE; i++) {
		(*koamgrp)->koa_tbl[i] = NULL;
	}

	if ((result = txc_pool_create(&((*koamgrp)->pool_buffer_dgram), 
															TXC_BUFFER_DGRAM_SIZE,
															TXC_BUFFER_DGRAM_NUM, NULL)) 
																									!= TXC_R_SUCCESS) {
		txc_pool_destroy(&((*koamgrp)->pool_koa_obj));
		free(*koamgrp);
		return result;
	}	

	if ((result = txc_pool_create(&((*koamgrp)->pool_buffer_bytestream), 
															TXC_BUFFER_BYTESTREAM_SIZE,
															TXC_BUFFER_BYTESTREAM_NUM, NULL)) 
																									!= TXC_R_SUCCESS) {
		txc_pool_destroy(&((*koamgrp)->pool_koa_obj));
		txc_pool_destroy(&((*koamgrp)->pool_buffer_dgram));
		free(*koamgrp);
		return result;
	}	

	return TXC_R_SUCCESS;
}

TM_WAIVER
void
txc_koamgr_destroy(txc_koamgr_t **koamgrp) {
	txc_pool_destroy(&((*koamgrp)->pool_koa_obj));
	free(*koamgrp);
	*koamgrp = NULL;
}

TM_WAIVER
txc_result_t
txc_koa_create(txc_koamgr_t *koamgr, txc_koa_t **koap, int type) {
	txc_koa_t *koa;
	txc_sentinelmgr_t *sentinelmgr;
	txc_sentinel_t *sentinel;
	txc_result_t result;

	if ((result = txc_pool_object_alloc(koamgr->pool_koa_obj,(void **) &koa)) 
																											!= TXC_R_SUCCESS) {
		assert(0);
		return result;
	}
	sentinelmgr = txc_transactionmgr->sentinelmgr;
	assert(txc_sentinel_create(sentinelmgr, &sentinel) == TXC_R_SUCCESS);
	pthread_mutex_init(&(koa->mutex), NULL); 
	koa->ref = sentinel->ref = 0;
	koa->sentinel = sentinel;
	koa->type = type;

	switch(type) {
		case TXC_KOA_IS_SOCK_DGRAM:
			if ((result = txc_pool_object_alloc(koamgr->pool_buffer_dgram, 
																		(void **) &(koa->buffer_input_dgram.buf))) 
										!= TXC_R_SUCCESS) {
				assert(0);
				return result;
			}
			koa->buffer_input_dgram.len = koamgr->pool_buffer_dgram->obj_size;
			koa->buffer_input_dgram.producer_primary_head = 0;
			koa->buffer_input_dgram.producer_primary_tail = 0;
			koa->buffer_input_dgram.producer_secondary_head = 0;
			koa->buffer_input_dgram.producer_secondary_tail = 0;
			break;
		case TXC_KOA_IS_PIPE_READ_END:
			if ((result = txc_pool_object_alloc(koamgr->pool_buffer_bytestream, 
															( void **) &(koa->buffer_input_bytestream.buf))) 
										!= TXC_R_SUCCESS) {
				assert(0);
				return result;
			}
			koa->buffer_input_bytestream.len = koamgr->pool_buffer_bytestream->obj_size;
			koa->buffer_input_bytestream.producer_primary_head = 0;
			koa->buffer_input_bytestream.producer_primary_tail = 0;
			koa->buffer_input_bytestream.producer_secondary_head = 0;
			koa->buffer_input_bytestream.producer_secondary_tail = 0;
			break;

		default:
			; /* empty statement */
	}

	*koap = koa;
	return TXC_R_SUCCESS;
}

TM_WAIVER
void
txc_koa_destroy(txc_koamgr_t *koamgr, txc_koa_t **koap) {
	txc_sentinelmgr_t *sentinelmgr;
	txc_sentinel_t *sentinel;

	sentinelmgr = txc_transactionmgr->sentinelmgr;
	txc_sentinel_destroy(&((*koap)->sentinel));
	switch((*koap)->type) {
		case TXC_KOA_IS_SOCK_DGRAM:
			txc_pool_object_free(
							txc_transactionmgr->koamgr->pool_buffer_dgram,
							(void **) &((*koap)->buffer_input_dgram.buf));
			break;
		case TXC_KOA_IS_PIPE_READ_END:
			txc_pool_object_free(
							txc_transactionmgr->koamgr->pool_buffer_bytestream,
							(void **) &((*koap)->buffer_input_bytestream.buf));
			break;
		default:
			; /* empty statement */
	}
	txc_pool_object_free(txc_transactionmgr->koamgr->pool_koa_obj, (void **) koap);
}

TM_WAIVER
txc_result_t
txc_koa_attach(txc_koamgr_t *koamgr, txc_koa_t *koa, int fd) {
	txc_sentinelmgr_t *sentinelmgr;

	assert(koa != NULL);
	if (koamgr->koa_tbl[fd] != NULL) {
		assert(0);
	}
	assert(koamgr->koa_tbl[fd] == NULL);
	assert(fd < TXC_KOA_TABLE_SIZE);
	pthread_mutex_lock(&(koa->mutex));
	koa->ref++;
	koa->sentinel->ref++;
	koamgr->koa_tbl[fd] = koa;
#ifdef _TXC_KOA_DEBUG	
	printf("txc_koa_attach: koa = %p, fd = %d, ref = %d, sentinel = %p\n", koa, fd, koa->ref, koa->sentinel);
#endif
	pthread_mutex_unlock(&(koa->mutex));

	return TXC_R_SUCCESS;
}

/*
 * txc_koa_detach destroys a KOA after detach if (references == 0)
 */

TM_WAIVER
txc_result_t
txc_koa_detach(txc_koamgr_t *koamgr, int fd) {
	txc_koa_t *koa;
	txc_sentinelmgr_t *sentinelmgr;
	int destroy_koa; 

	assert(fd < TXC_KOA_TABLE_SIZE);
	koa = txc_transactionmgr->koamgr->koa_tbl[fd];
	assert(koa != NULL);

	pthread_mutex_lock(&(koa->mutex));
	koa->ref--;
	koa->sentinel->ref--;
	koamgr->koa_tbl[fd] = NULL;
	destroy_koa = (koa->ref == 0) ? 1 : 0;
	pthread_mutex_unlock(&(koa->mutex));
#ifdef _TXC_KOA_DEBUG	
	printf("txc_koa_detach: koa = %p, fd = %d, sentinel = %p\n", koa, fd, koa->sentinel);
#endif
	if (destroy_koa) {
		txc_koa_destroy(txc_transactionmgr->koamgr, &koa);
	}
	return TXC_R_SUCCESS;
}

TM_WAIVER
txc_result_t
txc_koa_get(txc_koamgr_t *koamgr, txc_koa_t **koap, int fd) {
	assert(fd < TXC_KOA_TABLE_SIZE);
	*koap = koamgr->koa_tbl[fd];
	if (*koap == NULL) {
		return TXC_R_FAILURE;
	}
	return TXC_R_SUCCESS;
}
