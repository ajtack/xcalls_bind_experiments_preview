#include <txc/int/transaction.h>
#include <txc/int/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void 
txc_debug_internalerror(const char *string, char *file, int line)
{
	fprintf(stderr, "Internal error: %s at %s:%d\n", string, file, line);
	exit(-1);	
} 


void
txc_debug_printmsg(char *file, int line, int fatal, const char *prefix,
																					 const char *strformat, ...) 
{
	char buf[512];
  va_list ap;
	int len;
	if (prefix) {
		len = sprintf(buf, "%s ", prefix); 
	}
	if (file) {
		len += sprintf(&buf[len], "%s(%d)", file, line); 
	}
	if (file || prefix) {
		len += sprintf(&buf[len], ": "); 
	}
	va_start (ap, strformat);
  vsnprintf(&buf[len], sizeof (buf) - 1 - len, strformat, ap);
  va_end (ap);
	fprintf(TXC_DEBUG_OUT, "%s\n", buf);
	if (fatal) {
		exit(1);
	}
}


TM_WAIVER
void 
txc_debug_printmsg_L(int debug_level, const char *strformat, ...) 
{
	char msg[512];
	int len, cb; 
  va_list ap;
	struct timeval curtime;
	int xact_state;
	
	if (txc_debug_level < debug_level) {
		return;
	} 

	xact_state = txc_transaction_xact_state_get(); 
	gettimeofday(&curtime, NULL); 
	len = sprintf(msg, "[TXC_DEBUG: T-%02d (%u) %04d%06d TX=%d PC=%p] ", 
																			txc_xact_descriptor->tid, 
																			txc_xact_descriptor->stid, 
																			curtime.tv_sec, curtime.tv_usec,
																			xact_state,
																			__builtin_return_address(0)
																); 
	va_start (ap, strformat);
  vsnprintf(&msg[len], sizeof (msg) - 1 - len, strformat, ap);
  va_end (ap);

	len = strlen(msg);
	msg[len++] = '\n';
	msg[len] = '\0';

	fprintf(TXC_DEBUG_OUT, "%s", msg);
}
