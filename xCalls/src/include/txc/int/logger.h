#ifndef _TXC_LOGGER_H
#define _TXC_LOGGER_H

typedef struct txc_logger_private_log_s txc_logger_private_log_t;

/* Public interface functions */
#define TXC_PRIVLOG(msg, ...) 																							\
	txc_logger_private_logmsg(msg, ##__VA_ARGS__ )

#define TXC_GLOBLOG(msg, ...) 																							\
	txc_logger_global_logmsg(msg, ##__VA_ARGS__ )

TM_WAIVER void txc_logger_private_logmsg(const char *, ...); 
TM_WAIVER void txc_logger_global_logmsg(const char *, ...); 


#ifdef _TXC_LOGGER_H_PRIVATE_INTERFACE
/* Private interface functions */
void txc_logger_terminate();
txc_result_t 
txc_logger_private_create(txc_logger_private_log_t **logp, int tid);
txc_result_t  txc_logger_global_create();
#endif
#endif /* _TXC_LOGGER_H */
