#ifndef _TXC_DEBUG_H
#define _TXC_DEBUG_H

#include <assert.h>

#define TXC_DEBUG_OUT					stderr
#define txc_assert(condition) assert(condition) 


/* Operations on timevals */
#define timevalclear(tvp)      ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define timevaladd(vvp, uvp)                                            	\
        do {                                                            	\
                (vvp)->tv_sec += (uvp)->tv_sec;                         	\
                (vvp)->tv_usec += (uvp)->tv_usec;                       	\
                if ((vvp)->tv_usec >= 1000000) {                        	\
                        (vvp)->tv_sec++;                                	\
                        (vvp)->tv_usec -= 1000000;                      	\
                }                                                      		\
        } while (0)
#define timevalsub(vvp, uvp)                                            	\
        do {                                                            	\
                (vvp)->tv_sec -= (uvp)->tv_sec;                         	\
                (vvp)->tv_usec -= (uvp)->tv_usec;                       	\
                if ((vvp)->tv_usec < 0) {                               	\
                        (vvp)->tv_sec--;                                	\
                        (vvp)->tv_usec += 1000000;                      	\
                }                                                       	\
        } while (0)


#define TXC_INTERNALERROR(msg, ...) 																			\
	txc_debug_printmsg(__FILE__, __LINE__, 1, "Internal Error",							\
																							 msg, ##__VA_ARGS__ )

#define TXC_WARNING(msg, ...) 																						\
	txc_debug_printmsg(NULL, 0, 0, "Warning",	msg, ##__VA_ARGS__ )

#define TXC_ERROR(msg, ...) 																							\
	txc_debug_printmsg(NULL, 0, 1, "Error",	msg, ##__VA_ARGS__ )

TM_WAIVER void
txc_debug_printmsg(char *file, int line, int fatal, const char *prefix,
																					 const char *strformat, ...); 
TM_WAIVER void txc_debug_printmsg_L(int debug_level, const char *strformat, ...); 
#endif /* _TXC_DEBUG_H */
