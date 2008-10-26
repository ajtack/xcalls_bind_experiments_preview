#ifndef _TXC_PIPE_H
#define _TXC_PIPE_H

#define TXC_WRITE_PIPE_NOBUFFER					1

#define TXC_BUFFER_BYTESTREAM_SIZE			1024*(64*2)
#define TXC_BUFFER_BYTESTREAM_NUM				32



typedef struct txc_buffer_bytestream_s txc_buffer_bytestream_t;

/* Terminology could be misleadinng 
 * Producer here is the one that brings data inside the buffer, i.e.
 * a transactional read, and a consumer is the one that reads data from
 * the buffer, i.e. both transactional and non-transactional reader
 */ 
struct txc_buffer_bytestream_s {
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

#endif
