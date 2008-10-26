#ifndef _TXC_SOCKET_H
#define _TXC_SOCKET_H

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define TXC_DGRAM_MAX_SIZE							1024*64
#define TXC_CONTROLMSG_MAX_SIZE					1024*2
#define TXC_BUFFER_DGRAM_SIZE						1024*(64*2)*64
#define TXC_BUFFER_DGRAM_NUM						32

typedef struct txc_buffer_msghdr_s txc_buffer_msghdr_t;
typedef struct txc_buffer_dgram_s txc_buffer_dgram_t;

/* Terminology could be misleadinng 
 * Producer here is the one that brings data inside the buffer, i.e.
 * a transactional read, and a consumer is the one that reads data from
 * the buffer, i.e. both transactional and non-transactional reader
 */ 
struct txc_buffer_dgram_s {
	//char buf[TXC_BUFFER_DGRAM_SIZE];
	char *buf;
	int len;
	/* producer buffer pointers */
	int producer_primary_head;
	int producer_primary_tail;
	int producer_secondary_head;
	int producer_secondary_tail;
	/* consumer buffer pointers */
	int consumer_primary_head;
	int consumer_primary_tail;
	int consumer_secondary_head;
	int consumer_secondary_tail;
	
	int checkpointed;
};

struct txc_buffer_msghdr_s {
	struct sockaddr_in msg_name;
	socklen_t msg_namelen;
  char *msg_iov_base;
	size_t msg_iov_len;		/* length in bytes */
	void *msg_control;
	socklen_t msg_controllen;
	int msg_flags;
};

TM_WAIVER void txc_recvmsg_debug(int s);
#endif
