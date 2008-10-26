#include <txc/int/transaction.h>
#include <txc/int/debug.h>
#include <txc/libc.h>

//#define _TXC_SOCKET_DEBUG
TM_WAIVER ssize_t sendmsg(int s, const struct msghdr *msg, int flags);

TM_WAIVER
int 
txc_socket(int domain, int type,  int protocol) {
	txc_koa_t *koa;
	int fd;

	fd = socket(domain, type, protocol);
	switch(type) {
		case SOCK_DGRAM:
			assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, 
																TXC_KOA_IS_SOCK_DGRAM) == TXC_R_SUCCESS); 
			txc_koa_attach(txc_transactionmgr->koamgr, koa, fd);
			break;
		case SOCK_STREAM:
			assert(txc_koa_create(txc_transactionmgr->koamgr, &koa,
																TXC_KOA_IS_SOCK_STREAM) == TXC_R_SUCCESS); 
			txc_koa_attach(txc_transactionmgr->koamgr, koa, fd);
			break;
		default:
			txc_koa_create(txc_transactionmgr->koamgr, &koa, 0); 
			txc_koa_attach(txc_transactionmgr->koamgr, koa, fd);
	}
	return fd;
}


TM_WAIVER
int  
txc_getsockopt(int  s, int level, int optname, void *optval, socklen_t *optlen) {
	return getsockopt(s, level, optname, optval, optlen);
}

TM_WAIVER
int
txc_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen) {
	return setsockopt(s, level, optname, optval, optlen);
}

TM_WAIVER
ssize_t 
txc_recv(int s, void *buf, size_t len, int flags)
{
	//FIXME: This is the byte streaming version which we don't handle for now
	return recv(s, buf, len, flags);	
}

TM_WAIVER
ssize_t  
txc_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	//FIXME: This is the byte streaming version which we don't handle for now
	return recvfrom(s, buf, len, flags, from, fromlen);
}

TM_WAIVER
ssize_t       
txc_sendto(int s, const void *buf, size_t len, int flags, const
		struct sockaddr *to, socklen_t tolen)
{
	//FIXME: This is the byte streaming version which we don't handle for now
	return sendto(s, buf, len, flags, to, tolen);
}


TM_CALLABLE
ssize_t 
txc_sendmsg(int s, const struct msghdr *msg, int flags)
{
	//FIXME: sendmsg hasn't appeared in our workloads yet 
//	txc_stat_action_incr("txc_sendmsg", __FILE__, __LINE__);
	return sendmsg(s, msg, flags);
}

static void
__print_msg(txc_buffer_msghdr_t *msg) {
	int i, j;

	printf("\tFrom Address[len = %d]: %s\n", msg->msg_namelen, inet_ntoa(msg->msg_name.sin_addr));
	printf("\tMSG[base = %p, len = %d]: ", msg->msg_iov_base, msg->msg_iov_len);
	for (i=0;i<msg->msg_iov_len; i++) {
		printf("%c", ((char *) msg->msg_iov_base)[i]);
	}
	printf("\n");
}

void 
txc_recvmsg_compensating_action(unsigned int num_args, unsigned int *args)
{
	txc_koa_t *koa;
	txc_buffer_dgram_t *buffer;

	koa = (txc_koa_t *) args[0];
	buffer = &(koa->buffer_input_dgram);
	buffer->checkpointed = 0;
}

TM_WAIVER
static txc_result_t 
txc_recvmsg_commit_action(unsigned int num_args, unsigned int *args)
{
	txc_koa_t *koa;
	txc_buffer_dgram_t *buffer;

	koa = (txc_koa_t *) args[0];
	buffer = &(koa->buffer_input_dgram);

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
__txc_recvmsg(int fd, txc_koa_t *koa, struct msghdr *msg, int flags, 
									int speculative_read, int *txc_res)
{
	unsigned int arg_list[XACT_MAX_NUM_ACTION_ARGS];  
	int s, i, store_in_primary_buffer, store_in_secondary_buffer;
	struct iovec temp_iovec;
	struct msghdr temp_msghdr;
	txc_buffer_msghdr_t *temp_buffer_msghdr;
	int hdr_base_index, needed_bytes;
	txc_buffer_dgram_t *buffer;
	ssize_t result, len;

	char *base;

	buffer = &(koa->buffer_input_dgram);

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
				temp_buffer_msghdr = (txc_buffer_msghdr_t *) 
																					&(buffer->buf[hdr_base_index]);
				len = sizeof(txc_buffer_msghdr_t) + temp_buffer_msghdr->msg_iov_len + 
									temp_buffer_msghdr->msg_controllen;
#ifdef _TXC_SOCKET_DEBUG	
				printf("READ: Consume data from the primary buffer [%d, %d]\n", buffer->consumer_primary_head,	buffer->consumer_primary_head + len);
#endif
				buffer->producer_primary_head += len;
			} else {
				/* Consume data from the secondary buffer and make the secondary 
				 * buffer primary 
				 */
				hdr_base_index = buffer->producer_secondary_head;
				temp_buffer_msghdr = (txc_buffer_msghdr_t *) 
																					&(buffer->buf[hdr_base_index]);
				len = sizeof(txc_buffer_msghdr_t) + temp_buffer_msghdr->msg_iov_len + 
									temp_buffer_msghdr->msg_controllen;
#ifdef _TXC_SOCKET_DEBUG	
				printf("READ: Consume data from the secondary buffer [%d, %d]\n", buffer->consumer_secondary_head,	buffer->consumer_secondary_head + len);
#endif
				buffer->producer_secondary_head += len;
				buffer->producer_primary_tail = buffer->producer_secondary_tail; 
				buffer->producer_primary_head = buffer->producer_secondary_head; 
				buffer->producer_secondary_head = buffer->producer_secondary_tail = 0;
			}
			goto copy_back;	
		} else { 
			result = recvmsg(fd, msg, flags);
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
		txc_register_compensating_action(txc_recvmsg_compensating_action, 
																							1, arg_list);
		txc_register_commit_action(txc_recvmsg_commit_action, 1, arg_list);
	}

	if (buffer->consumer_primary_tail != buffer->consumer_primary_head ||
			buffer->consumer_secondary_tail != buffer->consumer_secondary_head) {
		/* There are buffered data available to consume*/
		if (buffer->consumer_primary_tail != buffer->consumer_primary_head) {
			/* Consume data from the primary buffer */
			hdr_base_index = buffer->consumer_primary_head;
			temp_buffer_msghdr = (txc_buffer_msghdr_t *) 
																					&(buffer->buf[hdr_base_index]);
			len = sizeof(txc_buffer_msghdr_t) + temp_buffer_msghdr->msg_iov_len + 
										temp_buffer_msghdr->msg_controllen;
#ifdef _TXC_SOCKET_DEBUG	
			printf("SPEC READ: Consume data from the primary buffer [%d, %d]\n", buffer->consumer_primary_head,	buffer->consumer_primary_head + len);
#endif
			buffer->consumer_primary_head += len;
		} else {
			/* Consume data from the secondary buffer */
			hdr_base_index = buffer->consumer_secondary_head;
			temp_buffer_msghdr = (txc_buffer_msghdr_t *) 
																					&(buffer->buf[hdr_base_index]);
#ifdef _TXC_SOCKET_DEBUG	
			printf("SPEC READ: Consume data from the secondary buffer [%d, %d]\n", buffer->consumer_secondary_head, len);
#endif
			buffer->consumer_secondary_head += len;
		}
	} else { 
		/* No buffered data available 
		 * 1) Find space in the buffer
		 * 2) Create a message header replica and save it in the buffer
		 * 3) Issue a recvmsg call to the kernel to bring the data in the buffer
		 * 4) Copy the buffered data back to the application buffer
		 */
		needed_bytes = sizeof(txc_buffer_msghdr_t) + 
														TXC_DGRAM_MAX_SIZE + TXC_CONTROLMSG_MAX_SIZE;
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
		temp_buffer_msghdr = (txc_buffer_msghdr_t *) 
																					&(buffer->buf[hdr_base_index]);
		temp_buffer_msghdr->msg_iov_base = 	(void *) (temp_buffer_msghdr + 
																						sizeof(txc_buffer_msghdr_t));
		temp_buffer_msghdr->msg_control = (void *) (temp_buffer_msghdr + 
																sizeof(txc_buffer_msghdr_t)) +
																TXC_DGRAM_MAX_SIZE;
		temp_iovec.iov_base = temp_buffer_msghdr->msg_iov_base; 
		temp_iovec.iov_len = TXC_DGRAM_MAX_SIZE;
		temp_msghdr.msg_name = (void *) &(temp_buffer_msghdr->msg_name);
		temp_msghdr.msg_namelen = sizeof(struct sockaddr_in);
		temp_msghdr.msg_iov = &temp_iovec;
		temp_msghdr.msg_iovlen = 1;
		temp_msghdr.msg_control = temp_buffer_msghdr->msg_control;
		temp_msghdr.msg_controllen = TXC_CONTROLMSG_MAX_SIZE;
		result = recvmsg(fd, &temp_msghdr, flags);
		if (result > 0) {
			temp_buffer_msghdr->msg_iov_len = result;
			temp_buffer_msghdr->msg_namelen = temp_msghdr.msg_namelen;
			temp_buffer_msghdr->msg_controllen = temp_msghdr.msg_controllen;
			temp_buffer_msghdr->msg_flags = temp_msghdr.msg_flags;
#ifdef _TXC_SOCKET_DEBUG	
			__print_msg(temp_buffer_msghdr);
#endif
			if (store_in_primary_buffer) {
				buffer->producer_primary_tail += sizeof(txc_buffer_msghdr_t) +
																					temp_buffer_msghdr->msg_iov_len +
																					temp_buffer_msghdr->msg_controllen;	
			} else if (store_in_secondary_buffer) {
				buffer->producer_secondary_tail += sizeof(txc_buffer_msghdr_t) +
																					temp_buffer_msghdr->msg_iov_len +
																					temp_buffer_msghdr->msg_controllen;	
			} else {
				assert(0); 	/* sanity check - shouldn't come here */
			}
			/* shift control message to the end of the msg_data */
			if (temp_buffer_msghdr->msg_controllen > 0) {
				memcpy((char *) temp_buffer_msghdr->msg_iov_base + 
																	temp_buffer_msghdr->msg_iov_len, 
														temp_buffer_msghdr->msg_control,
														temp_buffer_msghdr->msg_controllen);
			}
		}
	}
copy_back:
	/* Copy data from the buffer back to the application buffer */
	memcpy(msg->msg_name, (void *)&(temp_buffer_msghdr->msg_name), 
																						sizeof(struct sockaddr_in));
	msg->msg_namelen = temp_buffer_msghdr->msg_namelen;
	//hdr_index = hdr_base_index + sizeof(txc_buffer_msghdr_t);
	for (s=0, i=0; i<msg->msg_iovlen; i++) {
		memcpy(msg->msg_iov[i].iov_base, &temp_buffer_msghdr->msg_iov_base[s], 
																							msg->msg_iov[i].iov_len);
		s+=msg->msg_iov[i].iov_len;
	}
	msg->msg_flags = temp_buffer_msghdr->msg_flags;
	msg->msg_controllen = temp_buffer_msghdr->msg_controllen;
	memcpy(msg->msg_control, temp_buffer_msghdr->msg_control, 
																				temp_buffer_msghdr->msg_controllen);
	*txc_res = TXC_R_SUCCESS;
	return s;
}

TM_WAIVER
void
txc_recvmsg_debug(int s) {
	txc_koa_t *koa;
	txc_buffer_msghdr_t *temp_msg;
	unsigned int hdr_base_index, hdr_index;
	txc_buffer_dgram_t *buffer;
	ssize_t len;

	assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, s) == TXC_R_SUCCESS);
	buffer = &(koa->buffer_input_dgram);

	printf("=========================================\n");
	printf("koa->sentinel->id %d\n", koa->sentinel->id);
	printf("PRIMARY BUFFER\n");
	hdr_index = buffer->producer_primary_head;
	while (hdr_index != buffer->producer_primary_tail) {
		temp_msg = &buffer->buf[hdr_index];
		len = sizeof(txc_buffer_msghdr_t) + temp_msg->msg_iov_len + 
									temp_msg->msg_controllen;
		printf("Buffer[%d, %d]\n", hdr_index, hdr_index + len);
		__print_msg(temp_msg);
		hdr_index += len;
	}
	printf("SECONDARY BUFFER\n");
	hdr_index = buffer->producer_secondary_head;
	while (hdr_index != buffer->producer_secondary_tail) {
		temp_msg = &buffer->buf[hdr_index];
		len = sizeof(txc_buffer_msghdr_t) + temp_msg->msg_iov_len + 
									temp_msg->msg_controllen;
		printf("Buffer[%d, %d]\n", hdr_index, hdr_index + len);
		__print_msg(temp_msg);
		hdr_index += len;
	}
	printf("=========================================\n");
}

TM_CALLABLE
ssize_t 
txc_recvmsg(int s, struct msghdr *msg, int flags)
{
	int txc_res;
	ssize_t result; 	
	txc_koa_t *koa;

	assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, s) == TXC_R_SUCCESS);
//	txc_stat_action_incr("txc_recvmsg", __FILE__, __LINE__);
	switch (txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			result = __txc_recvmsg(s, koa, msg, flags, 0, &txc_res);
			SENTINEL_RELEASE(koa->sentinel);
			return result;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			result = __txc_recvmsg(s, koa, msg, flags, 0, &txc_res);
			return result;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			result = __txc_recvmsg(s, koa, msg, flags, 1, &txc_res);
			if (txc_res != TXC_R_SUCCESS) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				return result;
			}
		default:
			txc_assert(0);
	}
}
