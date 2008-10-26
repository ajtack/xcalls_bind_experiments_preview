#include <txc/int/transaction.h>
#include <txc/int/stat.h>
#include <txc/int/debug.h>
#include <unistd.h>
#include <errno.h>

TM_WAIVER int __pipe(int filedes[2]);
TM_WAIVER ssize_t __write(int fd, const void *buf, size_t count);
TM_WAIVER ssize_t __read(int fd, void *buf, size_t count);

TM_WAIVER 
int 
txc_pipe(int filedes[2]) {
	int result;
	txc_koa_t *koa;	
	
	result = __pipe(filedes);
	if (txc_transactionmgr && txc_transactionmgr->koamgr) {
		assert(txc_koa_create(txc_transactionmgr->koamgr, &koa,
															TXC_KOA_IS_PIPE_READ_END) == TXC_R_SUCCESS); 
 		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, filedes[0]) 
																											== TXC_R_SUCCESS);
		assert(txc_koa_create(txc_transactionmgr->koamgr, &koa,
															TXC_KOA_IS_PIPE_WRITE_END) == TXC_R_SUCCESS); 
 		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, filedes[1]) 
																											== TXC_R_SUCCESS);
	}
	return result;
}

TM_WAIVER 
int 
txc_pipe_read_end(int fd) {
	int result;
	txc_koa_t *koa;	
	
	if (txc_transactionmgr && txc_transactionmgr->koamgr) {
		assert(txc_koa_create(txc_transactionmgr->koamgr, &koa,
																TXC_KOA_IS_PIPE_READ_END) == TXC_R_SUCCESS); 
 		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fd) 
																											== TXC_R_SUCCESS);
	}
	return result;
}

TM_WAIVER 
int 
txc_pipe_write_end(int fd) {
	int result;
	txc_koa_t *koa;	
	
	if (txc_transactionmgr && txc_transactionmgr->koamgr) {
		assert(txc_koa_create(txc_transactionmgr->koamgr, &koa,
																TXC_KOA_IS_PIPE_WRITE_END) == TXC_R_SUCCESS); 
 		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fd) 
																											== TXC_R_SUCCESS);
	}
	return result;
}

void
__txc_write_pipe_commit(unsigned int num_inputs, unsigned int* input_array) {
	int fildes = (int) input_array[0];
	void *local_buf = (void *) input_array[1];
	size_t nbyte = (size_t) input_array[2];

	__write(fildes, local_buf, nbyte);
}
	
TM_WAIVER
ssize_t
__txc_write_pipe(int fildes, const void *buf, size_t nbyte, int speculative_write, int flag) {
	unsigned int input_array[6];
	char *copied_buf;

	if (!speculative_write) {
		return __write(fildes, buf, nbyte);
	} else {
		input_array[0] = fildes;
		input_array[2] = (unsigned int) nbyte;
		if (flag == TXC_WRITE_PIPE_NOBUFFER) {
			input_array[1] = (unsigned int) buf;
		} else {
			//FIXME: If it fails then serialize
			assert(txc_transaction_log_buffer_allocate(nbyte*sizeof(char),
																		(void **) &copied_buf) == TXC_R_SUCCESS); 
			memcpy(copied_buf, buf, nbyte);
			input_array[1] = (unsigned int) copied_buf;
		}
		txc_register_commit_action(&__txc_write_pipe_commit, 3, input_array);
	}
	return nbyte;
}
	
TM_CALLABLE
ssize_t
txc_write_pipe(int fildes, const void *buf, size_t nbyte, int flag)
{
	txc_koa_t *koa;	
	int ret;

	if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
		assert(0);		
	}

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __txc_write_pipe(fildes, buf, nbyte, 0, flag);
			SENTINEL_RELEASE(koa->sentinel);
			return ret;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_write_pipe(fildes, buf, nbyte, 1, flag);
			if (ret < 0) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				return ret;
			}
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			return __txc_write_pipe(fildes, buf, nbyte, 0, flag);
		default:
			assert(0);
	}
}

void 
__txc_read_pipe_compensating_action(unsigned int num_args, unsigned int *args) 
{
	txc_koa_t *koa;
	txc_buffer_bytestream_t *buffer;

	koa = (txc_koa_t *) args[0];
	buffer = &(koa->buffer_input_bytestream);
	buffer->checkpointed = 0;
}

TM_WAIVER
static txc_result_t 
__txc_read_pipe_commit_action(unsigned int num_args, unsigned int *args)
{
	txc_koa_t *koa;
	txc_buffer_bytestream_t *buffer;

	koa = (txc_koa_t *) args[0];
	buffer = &(koa->buffer_input_bytestream);

	if (buffer->consumer_primary_tail != buffer->consumer_primary_head) {
		buffer->producer_primary_head = buffer->consumer_primary_head; 
		buffer->producer_primary_tail = buffer->consumer_primary_tail; 
	} else if (buffer->consumer_secondary_tail != buffer->consumer_secondary_head) {
		buffer->producer_primary_head = buffer->consumer_secondary_head; 
		buffer->producer_primary_tail = buffer->consumer_secondary_tail; 
		buffer->producer_secondary_head = buffer->producer_secondary_tail = 0;
	} else {
		buffer->producer_primary_head = buffer->producer_primary_tail = 0;
		buffer->producer_secondary_head = buffer->producer_secondary_tail = 0;
	}
	buffer->checkpointed = 0;
}


TM_WAIVER
static ssize_t 
__txc_read_pipe(int fd, txc_koa_t *koa, void *buf, size_t nbyte, 
																			int speculative_read, int *txc_res) {
	unsigned int arg_list[XACT_MAX_NUM_ACTION_ARGS];  
	int s, store_in_primary_buffer, store_in_secondary_buffer;
	int hdr_base_index, needed_bytes;
	txc_buffer_bytestream_t *buffer;
	ssize_t result;

	buffer = &(koa->buffer_input_bytestream);
	
	// FIXME: We don't handle the case where we need to consume more data 
	// than available in the buffer. In such a case you need to request
	// the additional data

	if (!speculative_read) {
		//FIXME:  pthread_mutex_lock(buffer->mutex);
	}

	if (!speculative_read) {
		if (buffer->producer_primary_tail != buffer->producer_primary_head	||
				buffer->producer_secondary_tail != buffer->producer_secondary_head) {
			/* There are buffered data available */
			if (buffer->producer_primary_tail != buffer->producer_primary_head) {
				/* Consume data from the primary buffer */
				hdr_base_index = buffer->producer_primary_head;
				buffer->producer_primary_head += nbyte;
			} else {
				/* Consume data from the secondary buffer and make the secondary 
				 * buffer primary 
				 */
				hdr_base_index = buffer->producer_secondary_head;
				buffer->producer_secondary_head += nbyte;
				buffer->producer_primary_tail = buffer->producer_secondary_tail; 
				buffer->producer_primary_head = buffer->producer_secondary_head; 
				buffer->producer_secondary_head = buffer->producer_secondary_tail = 0;
			}
			goto copy_back;	
		} else { 
			result = __read(fd, buf, nbyte);
			return result;	
		}
	}

	/* 
	 * READ is speculative
	 */		

	if (buffer->checkpointed == 0) {
		buffer->consumer_primary_head = buffer->producer_primary_head;
		buffer->consumer_primary_tail = buffer->producer_primary_tail;
		buffer->consumer_secondary_head = buffer->producer_secondary_head;
		buffer->consumer_secondary_tail = buffer->producer_secondary_tail;
		buffer->checkpointed = 1;
		arg_list[0] = (unsigned int) koa;
		txc_register_compensating_action(__txc_read_pipe_compensating_action, 
																							1, arg_list);
		txc_register_commit_action(__txc_read_pipe_commit_action, 1, arg_list);
	}

	if (buffer->consumer_primary_tail != buffer->consumer_primary_head ||
			buffer->consumer_secondary_tail != buffer->consumer_secondary_head) {
		/* There are buffered data available to consume*/
		if (buffer->consumer_primary_tail != buffer->consumer_primary_head) {
			/* Consume data from the primary buffer */
			hdr_base_index = buffer->consumer_primary_head;
			buffer->consumer_primary_head += nbyte;
		} else {
			/* Consume data from the secondary buffer */
			hdr_base_index = buffer->consumer_secondary_head;
			buffer->consumer_secondary_head += nbyte;
		}
	} else { 
		/* No buffered data available 
		 * 1) Find space in the buffer
		 * 3) Issue a read call to the kernel to bring the data in the buffer
		 * 4) Copy the buffered data back to the application buffer
		 */
		needed_bytes = nbyte; 
		store_in_primary_buffer = store_in_secondary_buffer = 0;
		if (buffer->producer_primary_tail < (buffer->len - needed_bytes)) {
			/* There is space in the producer_primary part of the buffer */
			hdr_base_index = buffer->producer_primary_tail;
			store_in_primary_buffer = 1;
		}	else if (buffer->producer_secondary_tail < 
												(buffer->producer_primary_head + 1 - needed_bytes)) {
			/* There is space in the producer_secondary part of the buffer */
			hdr_base_index = buffer->producer_secondary_tail;
			store_in_secondary_buffer = 1;
		} else {
			/* There is not space in the buffer */
			*txc_res = TXC_R_NOMEMORY;
			return -1;
		}
		result = read(fd, &(buffer->buf[hdr_base_index]), nbyte);
		if (result > 0) {
			if (store_in_primary_buffer) {
				buffer->producer_primary_tail += result;
			} else if (store_in_secondary_buffer) {
				buffer->producer_secondary_tail += result;
			} else {
				assert(0); 	/* sanity check - shouldn't come here */
			}
		}
	}
copy_back:
	/* Copy data from the buffer back to the application buffer */
	memcpy(buf, &buffer->buf[hdr_base_index], nbyte); 
	*txc_res = TXC_R_SUCCESS;
	return s;
}

TM_CALLABLE
ssize_t 
txc_read_pipe(int fildes, void *buf, size_t nbyte)
{
	int txc_res;
	ssize_t result; 	
	txc_koa_t *koa;

	assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
//	txc_stat_action_incr("txc_read_pipe", __FILE__, __LINE__);
	switch (txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			result = __txc_read_pipe(fildes, koa, buf, nbyte, 0, &txc_res);
			SENTINEL_RELEASE(koa->sentinel);
			return result;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			result = __txc_read_pipe(fildes, koa, buf, nbyte, 0, &txc_res);
			return result;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			result = __txc_read_pipe(fildes, koa, buf, nbyte, 1, &txc_res);
			if (txc_res != TXC_R_SUCCESS) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				return result;
			}
		default:
			txc_assert(0);
	}
}

