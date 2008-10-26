#ifndef _TXC_LIBC_H
#define _TXC_LIBC_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <nl_types.h>
#include <stdarg.h>
#include <iconv.h>

#define HUGE    1.701411733192644270e38
#define LOGHUGE 39

//FIXME: NEELAM
#define ic_from(x)  (((x))&0xffff)
#define ic_to(x)  (((x)>>16)&0xffff)

# define __ptr_t    char * /* Not C++ or ANSI C.  */
//# define __ptr_t    void *  /* C++ or ANSI C.  */
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

/* Flags for file I/O */
#define	_IOREAD	01
#define	_IOWRT	02
#define	_IONBF	04
#define	_IOMYBUF	010
#define	_IOEOF	020
#define	_IOERR	040
#define	_IOSTRG	0100
#define	_IORW	0200
#define	EOF	(-1)
#define	_NFILE	20

struct catalog_obj
{
	u_int32_t magic;
	u_int32_t plane_size;
	u_int32_t plane_depth;
	/* This is in fact two arrays in one: always a pair of name and
	 *pointer into the data area.  */
	u_int32_t name_ptr[0];
};


/* This structure will be filled after loading the catalog.  */
typedef struct catalog_info
{
	enum { mmapped, malloced } status;
    size_t plane_size;
    size_t plane_depth;
    u_int32_t *name_ptr;
    const char *strings;
    struct catalog_obj *file_ptr;
    size_t file_size;
} *__nl_catd;

enum charset {
	INVALID=0,
	ISO_8859_1,
	UTF_8,
	UCS_2,
	UCS_4,
	UTF_16_BE,
	UTF_16_LE,
	UTF_16
};

TM_CALLABLE uint16_t htons (uint16_t x);
TM_CALLABLE uint32_t htonl (uint32_t x);
TM_WAIVER int inet_pton(int af, const char *src, void *dst);
TM_CALLABLE int inet_aton(const char *cp, struct in_addr *addr);
TM_WAIVER int pthread_mutex_lock(pthread_mutex_t *mutex);
TM_WAIVER int pthread_mutex_unlock(pthread_mutex_t *mutex);
TM_CALLABLE int stm_isdigit(int ch);
TM_CALLABLE int atoi(const char *str);
TM_CALLABLE double atof(char *p);
TM_CALLABLE char *strcpy(char *dest, const char *src);
TM_CALLABLE int strcmp(const char * a, const char * b);
TM_CALLABLE int strcasecmp (const char *s1, const char *s2);
TM_CALLABLE size_t strlen(char *str); 
TM_CALLABLE void* memchr(const void *s, int c, size_t n);
TM_CALLABLE void *memset(void* buffer, int ch, size_t count);
TM_CALLABLE void *memcpy(void* dstpp, const void* srcpp, size_t len);
TM_CALLABLE int memcmp (const __ptr_t s1, const __ptr_t s2, size_t len);
TM_CALLABLE void* memmove (const void* a1, const void* a2, size_t len);
TM_WAIVER	pid_t getpid(void);
TM_WAIVER	void usleep(unsigned long usec);
TM_WAIVER	int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
TM_WAIVER	int txc_rand(void);
TM_CALLABLE const char * inet_ntop(int af, const void *src, char *dst, socklen_t size);
TM_CALLABLE char *strrchr (const char *s, int c);
TM_CALLABLE char *strchr (const char *s, int c_in);
TM_CALLABLE int strncmp (const char *s1, const char *s2, size_t n);
TM_CALLABLE char *strncpy (char *s1, const char *s2, size_t n);
TM_CALLABLE char *strcat (char *dest, const char *src);
TM_CALLABLE char *strncat (char *dest, const char *src, size_t n);
TM_CALLABLE int strncasecmp(const char *s1, const char *s2,  size_t n);
TM_CALLABLE size_t iconv(iconv_t cd, char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t * outbytesleft);
TM_CALLABLE iconv_t iconv_open(const char* tocode, const char* fromcode);
TM_CALLABLE int iconv_close(iconv_t cd);
TM_WAIVER	int *txc_alias_errno_location(void);
TM_CALLABLE char *txc_alias_catgets (nl_catd catalog_desc, int set, int message, const char *string);

#endif
