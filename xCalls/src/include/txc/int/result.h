#ifndef _TXC_RESULT_H
#define _TXC_RESULT_H

#define TXC_R_SUCCESS			0	/*%< success */
#define TXC_R_NOMEMORY			1	/*%< out of memory */
#define TXC_R_TIMEDOUT			2	/*%< timed out */
#define TXC_R_NOTHREADS			3	/*%< no available threads */
#define TXC_R_ADDRNOTAVAIL		4	/*%< address not available */
#define TXC_R_ADDRINUSE			5	/*%< address in use */
#define TXC_R_NOPERM			6	/*%< permission denied */
#define TXC_R_NOCONN			7	/*%< no pending connections */
#define TXC_R_NETUNREACH		8	/*%< network unreachable */
#define TXC_R_HOSTUNREACH		9	/*%< host unreachable */
#define TXC_R_NETDOWN			10	/*%< network down */
#define TXC_R_HOSTDOWN			11	/*%< host down */
#define TXC_R_CONNREFUSED		12	/*%< connection refused */
#define TXC_R_NORESOURCES		13	/*%< not enough free resources */
#define TXC_R_EOF			14	/*%< end of file */
#define TXC_R_BOUND			15	/*%< socket already bound */
#define TXC_R_RELOAD			16	/*%< reload */
#define TXC_R_LOCKBUSY			17	/*%< lock busy */
#define TXC_R_EXISTS			18	/*%< already exists */
#define TXC_R_NOSPACE			19	/*%< ran out of space */
#define TXC_R_CANCELED			20	/*%< operation canceled */
#define TXC_R_NOTBOUND			21	/*%< socket is not bound */
#define TXC_R_SHUTTINGDOWN		22	/*%< shutting down */
#define TXC_R_NOTFOUND			23	/*%< not found */
#define TXC_R_UNEXPECTEDEND		24	/*%< unexpected end of input */
#define TXC_R_FAILURE			25	/*%< generic failure */
#define TXC_R_IOERROR			26	/*%< I/O error */
#define TXC_R_NOTIMPLEMENTED		27	/*%< not implemented */
#define TXC_R_UNBALANCED		28	/*%< unbalanced parentheses */
#define TXC_R_NOMORE			29	/*%< no more */
#define TXC_R_INVALIDFILE		30	/*%< invalid file */
#define TXC_R_BADBASE64			31	/*%< bad base64 encoding */
#define TXC_R_UNEXPECTEDTOKEN		32	/*%< unexpected token */
#define TXC_R_QUOTA			33	/*%< quota reached */
#define TXC_R_UNEXPECTED		34	/*%< unexpected error */
#define TXC_R_ALREADYRUNNING		35	/*%< already running */
#define TXC_R_IGNORE			36	/*%< ignore */
#define TXC_R_MASKNONCONTIG             37	/*%< addr mask not contiguous */
#define TXC_R_FILENOTFOUND		38	/*%< file not found */
#define TXC_R_FILEEXISTS		39	/*%< file already exists */
#define TXC_R_NOTCONNECTED		40	/*%< socket is not connected */
#define TXC_R_RANGE			41	/*%< out of range */
#define TXC_R_NOENTROPY			42	/*%< out of entropy */
#define TXC_R_MULTICAST			43	/*%< invalid use of multicast */
#define TXC_R_NOTFILE			44	/*%< not a file */
#define TXC_R_NOTDIRECTORY		45	/*%< not a directory */
#define TXC_R_QUEUEFULL			46	/*%< queue is full */
#define TXC_R_FAMILYMISMATCH		47	/*%< address family mismatch */
#define TXC_R_FAMILYNOSUPPORT		48	/*%< AF not supported */
#define TXC_R_BADHEX			49	/*%< bad hex encoding */
#define ISC_R_TOOMANYOPENFILES    50  /*%< too many open files */
#define ISC_R_NOTBLOCKING   51  /*%< not blocking */
#define ISC_R_UNBALANCEDQUOTES    52  /*%< unbalanced quotes */
#define ISC_R_INPROGRESS    53  /*%< operation in progress */
#define ISC_R_CONNECTIONRESET   54  /*%< connection reset */
#define ISC_R_SOFTQUOTA     55  /*%< soft quota reached */
#define ISC_R_BADNUMBER     56  /*%< not a valid number */
#define ISC_R_DISABLED      57  /*%< disabled */
#define ISC_R_MAXSIZE     58  /*%< max size */
#define ISC_R_BADADDRESSFORM    59  /*%< invalid address format */
#define TXC_R_NOTINITLOCK			60	/*%< could not initialize lock */
#define TXC_R_STATNOTENABLED		61	/*%< statistics not enabled */
#define TXC_R_STATINVALIDPROBE	62	/*%< invalid probe  */
#define TXC_R_SENTINELBUSY			63	/*%< invalid probe  */
#define TXC_R_PRIVLOGGERNOTENABLED		64	/*%< per thread private logger not enabled */
#define TXC_R_GLOBLOGGERNOTENABLED		65	/*%< per thread global logger not enabled */
 	
/*% Not a result code: the number of results. */
#define TXC_R_NRESULTS      66

typedef unsigned int			txc_result_t;		/*%< Result */

#endif

