/* Fast Loggger
 * ============
 * Since we are dealing with multithreaded applications, our logger 
 * implementation should be as less intrusive as possible resulting
 * in the following design points: 
 *  1) No synchronization, no interlock operations ==> per thread
 *     private structures.
 *  2) Minimize memory copies ==> contiguous buffers and not ring ones
 *  3) Asynchronous I/O for flushing the buffer in a file
 * 
 * Using per thread private buffer implies that a total ordering of the
 * events has 1 usec granularity. That is, events which appear to happen
 * at the same microsecond cannot be ordered to each other.      
 */

#define TXC_LOGGER_PRIVATE_LOG_NUM_BUFFERS 2
#define TXC_LOGGER_PRIVATE_LOG_BUFFER_SIZE 1024*256
#define TXC_LOGGER_GLOBAL_LOG_BUFFER_SIZE 1024*256

#include <txc/int/transaction.h>
#include <txc/int/rtconfig.h>
#include <txc/int/logger.h>
#include <txc/int/debug.h>
#include <txc/int/atomic.h>
#include <aio.h>
#include <stdarg.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>

typedef struct txc_logger_private_log_buffer_s txc_logger_private_log_buffer_t;
typedef struct txc_logger_global_log_s txc_logger_global_log_t; 

struct txc_logger_private_log_buffer_s {
	char *buf;
	char *marker;
	char *limit;
	int clear;
	struct aiocb aiocb_write; 
};

struct txc_logger_private_log_s {
	int tid;
	int num_buffers;
	int current_buffer;
	txc_logger_private_log_buffer_t buffers[TXC_LOGGER_PRIVATE_LOG_NUM_BUFFERS];
};

struct txc_logger_global_log_s {
	char *buf;
	char *limit;
	char *marker;
	int flushing;
	int fd;
};

static txc_logger_global_log_t globlog;
static unsigned int globlog_enabled = 0;

void
txc_logger_terminate() 
{
	int i, j, cb, num_threads;
	txc_logger_private_log_t *log;

	num_threads = txc_transactionmgr->num_threads;
	for (i=0; i<num_threads; i++) {
		if (log=txc_transactionmgr->xact_descriptor_tbl[i]->logger_private_log) {
			/* flush current buffer and wait until all outstanding flushes finish */
			cb = log->current_buffer;
			if (log->buffers[cb].clear == 0) {
				log->buffers[cb].aiocb_write.aio_buf = log->buffers[cb].buf;
			  log->buffers[cb].aiocb_write.aio_nbytes = 
																							strlen(log->buffers[cb].buf); 
		  	log->buffers[cb].aiocb_write.aio_reqprio = 0;
		    if (aio_write (&(log->buffers[cb].aiocb_write)) == -1) {
						TXC_ERROR("aio: write failed"); 
				}
			}
			for (j=0; j<log->num_buffers; j++) {
				while (aio_error (&(log->buffers[j].aiocb_write)) == EINPROGRESS);
			}
			close(log->buffers[0].aiocb_write.aio_fildes);
		}
	}
}

/*
 * PRIVATE LOG IMPLEMENTATION
 */

txc_result_t 
txc_logger_private_create(txc_logger_private_log_t **logp, int tid) {
	char log_filename[128];
	int fd;

	if (strcmp(txc_runtime_settings.privlogger, "disabled") == 0) {
		*logp = NULL;
		return TXC_R_PRIVLOGGERNOTENABLED;
	}	   
	*logp = (txc_logger_private_log_t *) 
												malloc(sizeof(txc_logger_private_log_t));
	if (*logp == NULL) {
		return TXC_R_NOMEMORY;
	}
	(*logp)->tid = tid;
	(*logp)->num_buffers = TXC_LOGGER_PRIVATE_LOG_NUM_BUFFERS;
	(*logp)->current_buffer = 0;
	snprintf(log_filename, 128, "%s_T%d", 
														txc_runtime_settings.privlogger_fileprefix, tid);
	if ((fd = open(log_filename, O_CREAT | O_TRUNC | O_WRONLY| O_APPEND, 0666))
				 < 0) { 
		TXC_ERROR("aio: cannot open %s errno 0x%x", log_filename, errno); 
  }	
	for (int i=0; i<(*logp)->num_buffers; i++) {
	 	(*logp)->buffers[i].buf = 
										(char *) malloc(TXC_LOGGER_PRIVATE_LOG_BUFFER_SIZE);
		(*logp)->buffers[i].marker = (*logp)->buffers[i].buf;
		(*logp)->buffers[i].limit = (*logp)->buffers[i].buf + 
																		TXC_LOGGER_PRIVATE_LOG_BUFFER_SIZE;
		(*logp)->buffers[i].clear = 0;
  	bzero((char *) &((*logp)->buffers[i].aiocb_write), sizeof(struct aiocb)); 
  	(*logp)->buffers[i].aiocb_write.aio_fildes = fd; 
	}

	return TXC_R_SUCCESS;
} 

void
txc_logger_private_logmsg(const char *strformat, ...) 
{
	char msg[1024];
	int len, done, cb; 
  va_list ap;
	struct timeval curtime;
	txc_logger_private_log_t *log;

	if ((log = txc_xact_descriptor->logger_private_log) == NULL) {
		return;
	}
	
	gettimeofday(&curtime, NULL); 
	timevalsub(&curtime, &txc_initialization_time);
	len = sprintf(msg, "[T-%02d %04d%06d] ", log->tid, 
																			curtime.tv_sec, curtime.tv_usec); 
	va_start (ap, strformat);
  vsnprintf(&msg[len], sizeof (msg) - 1 - len, strformat, ap);
  va_end (ap);

	len = strlen(msg);
	
	done = 0;
	while (!done) {
		cb = log->current_buffer;
		if ((aio_error (&(log->buffers[cb].aiocb_write)) != EINPROGRESS)) {
			if (log->buffers[cb].clear) {
				log->buffers[cb].clear = 0;
				log->buffers[cb].marker = log->buffers[cb].buf;
			}
			if (log->buffers[cb].marker + len < log->buffers[cb].limit) {
				strcpy(log->buffers[cb].marker, msg);
				log->buffers[cb].marker +=len; 
				done = 1;
			} else {
				log->buffers[cb].aiocb_write.aio_buf = log->buffers[cb].buf; 
			  log->buffers[cb].aiocb_write.aio_nbytes = strlen (log->buffers[cb].buf); 
		  	log->buffers[cb].aiocb_write.aio_reqprio = 0;
				log->buffers[cb].clear = 1;
		    if (aio_write (&(log->buffers[cb].aiocb_write)) == -1) {
					TXC_ERROR("aio: write failed"); 
				}
			}	
		} else {
			log->current_buffer = (log->current_buffer + 1) % log->num_buffers;	
		}
	}
}

/*
 * GLOBAL LOG IMPLEMENTATION
 */

txc_result_t 
txc_logger_global_create() {
	char log_filename[128];
	int fd;

	if (strcmp(txc_runtime_settings.globlogger, "disabled") == 0) {
		globlog_enabled = 0;
		return TXC_R_GLOBLOGGERNOTENABLED;
	}	else {   
		globlog_enabled = 1;
	}
	snprintf(log_filename, 128, "%s", 
														txc_runtime_settings.globlogger_fileprefix);
	if ((globlog.fd = open(log_filename, O_CREAT | O_TRUNC | O_WRONLY| O_APPEND, 0666))
				 < 0) { 
		TXC_ERROR("globlog: cannot open %s errno 0x%x", log_filename, errno); 
  }	
 	globlog.buf = (char *) malloc(TXC_LOGGER_GLOBAL_LOG_BUFFER_SIZE);
	globlog.marker = globlog.buf;
	globlog.flushing = 0;
	globlog.limit = globlog.buf + TXC_LOGGER_GLOBAL_LOG_BUFFER_SIZE;

	return TXC_R_SUCCESS;
}

void
flush_globlog() 
{
  if (txc_atomic_cmpxchg(&globlog.flushing, 0, 1) == 0) {
    write (globlog.fd, globlog.buf, globlog.marker-globlog.buf);
    globlog.marker = globlog.buf;
		// FIXME: To make sure the log is written:	fsync(globlog.fd);
    // but then this becomes a significant overhead 
    globlog.flushing = 0;
  }
}

void
txc_logger_global_logmsg(const char *strformat, ...) 
{
	char msg[512];
	int len; 
  va_list ap;
	struct timeval curtime;

	if (globlog_enabled == 0) {
		return;
	}
	
	gettimeofday(&curtime, NULL); 
	timevalsub(&curtime, &txc_initialization_time);
	len = sprintf(msg, "[T-%02d %04d%06d] ", txc_xact_descriptor->tid, 
																			curtime.tv_sec, curtime.tv_usec); 
	va_start (ap, strformat);
  vsnprintf(&msg[len], sizeof (msg) - 1 - len, strformat, ap);
  va_end (ap);

	len = strlen(msg);
	msg[len++] = '\n';
	msg[len] = '\0';

  char *loc;

  for (;;) {
	  char *new_loc;
    loc = globlog.marker;
    new_loc = loc + len;
    if (new_loc >= (globlog.limit)) {
	    flush_globlog ();
      continue;
    }
    if (txc_atomic_cmpxchg (&globlog.marker, (int) loc, (int) new_loc) 
																												== (int) loc) {
      break;
    }
  }
  strncpy (loc, msg, len);
}
