#ifndef _TXC_XCALLS_H
#define _TXC_XCALLS_H

#include <pthread.h>
#include <sys/socket.h>

/* Constant definitions */

/* Last parameter to be passed to the txc_write xCall */
#define APPEND_ONLY		0
#define OW_IGNORE		1
#define OW_SAVE			2


/* xCall interface */

//TM_WAIVER void txc_va_start(va_list ap, const char* format);
//TM_WAIVER void txc_va_end(va_list ap);
TM_CALLABLE void *txc_malloc(size_t size);
TM_CALLABLE void txc_free(void *ptr);

/* I/O */
TM_CALLABLE ssize_t txc_write(int fildes, const void *buf, size_t nbyte, int flag);
TM_CALLABLE ssize_t txc_pwrite(int fildes, const void *buf, size_t nbyte, off_t offset);
TM_CALLABLE ssize_t txc_read(int fildes, void *buf, size_t nbyte);
TM_CALLABLE ssize_t txc_pread(int fildes, void *buf, size_t nbyte, off_t offset);
TM_CALLABLE off_t txc_lseek(int fildes, off_t offset, int whence);
TM_WAIVER off64_t txc_lseek64(int fildes, off64_t offset, int whence);
TM_CALLABLE int txc_close(int fildes);
TM_CALLABLE int txc_open(const char *pathname, int flags, mode_t mode);
TM_WAIVER int txc_creat(const char *path, mode_t mode);
TM_CALLABLE int txc_unlink(const char *path);
TM_WAIVER int txc_chmod(const char *path, mode_t mode);
TM_CALLABLE int txc_rename(const char *old, const char *newname);
TM_WAIVER int txc_fcntl(int fd, int cmd, long arg);
TM_WAIVER int txc_fsync(int fd);
TM_WAIVER int txc_fdatasync(int fd);
TM_CALLABLE int txc_mkdir(const char *pathname, mode_t mode);

/* stdio */
#if 0
TM_CALLABLE FILE *txc_fopen(char *file, char *mode);
TM_CALLABLE size_t txc_fread(char *ptr, unsigned int size, unsigned int count, FILE *iop);
TM_CALLABLE size_t txc_fwrite(char *ptr, unsigned int size, unsigned int count, FILE *iop);
TM_CALLABLE char *txc_fgets(char *s, int n, FILE *iop);
TM_CALLABLE int txc_fclose(FILE *stream);
#endif

TM_WAIVER char *txc_strerror(int errnum);
/* pipe */
TM_WAIVER int txc_pipe(int filedes[2]);
TM_WAIVER int txc_pipe_read_end(int fd);
TM_CALLABLE ssize_t txc_write_pipe(int fildes, const void *buf, size_t nbyte, int flag);
TM_CALLABLE ssize_t txc_read_pipe(int fildes, void *buf, size_t nbyte);

/* socket */
TM_WAIVER int txc_socket(int domain, int type,  int protocol);
TM_WAIVER int getsockname(int s, struct sockaddr *name, socklen_t *namelen);
TM_WAIVER int txc_getsockopt(int  s, int level, int optname, void *optval, socklen_t *optlen);
TM_WAIVER	int	txc_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen); 
TM_WAIVER	ssize_t txc_recv(int s, void *buf, size_t len, int flags);
TM_CALLABLE ssize_t txc_recvmsg(int s, struct msghdr *msg, int flags);
TM_WAIVER ssize_t txc_recvfrom (int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
TM_WAIVER	ssize_t txc_sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
TM_CALLABLE ssize_t txc_sendmsg(int s, struct msghdr *msg, int flags);

/* printf and variants */
TM_CALLABLE int txc_printf(const char *format, ...);
TM_CALLABLE int txc_fprintf(FILE *stream, const char *format, ...);
TM_CALLABLE int txc_sprintf(char *str, const char *format, ...);
TM_CALLABLE int txc_snprintf(char *str, size_t size, const char *format,    ...);
TM_CALLABLE int txc_vprintf(const char *format, va_list ap);
TM_CALLABLE int txc_vfprintf(FILE *stream, const        char *format, va_list ap);
TM_CALLABLE int txc_vsprintf(char *str, const char *format, va_list ap);
TM_CALLABLE int txc_vsnprintf(char *str, size_t size, const char   *format, va_list ap);

/* Process */
TM_WAIVER pid_t txc_getpid(void);
TM_WAIVER pid_t txc_getppid(void);

/* POSIX Threads */
TM_WAIVER pthread_t txc_pthread_self(void); 

/* Transaction Safe Locks and Conditional Synchronization */
TM_WAIVER int txc_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
TM_CALLABLE int txc_pthread_mutex_lock(pthread_mutex_t *mutex);
TM_WAIVER int txc_pthread_mutex_unlock(pthread_mutex_t *mutex);
TM_CALLABLE int txc_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t *attr);
TM_CALLABLE int txc_pthread_cond_wait_enqueue(pthread_cond_t* cond);
TM_WAIVER int txc_pthread_cond_wait_block(pthread_cond_t* cond);
TM_CALLABLE int txc_pthread_cond_signal(pthread_cond_t* cond);

/* Memory allocation */
TM_CALLABLE void *txc_malloc(size_t size); 
TM_CALLABLE void *txc_calloc(size_t nmemb, size_t size);
TM_CALLABLE void txc_free(void *ptr);
TM_CALLABLE void *txc_realloc(void *ptr, size_t size);

/* Not-categorized */
TM_WAIVER int txc_gettimeofday(struct timeval *tv, struct timezone *tz);

#endif	/* _TXC_XCALLS_H
