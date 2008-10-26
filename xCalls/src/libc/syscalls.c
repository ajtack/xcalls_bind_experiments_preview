#include <txc/int/transaction.h>
#include <sys/types.h>
#include <dlfcn.h>

#define ACTION(name, ret_type, signature, args, print)			\
ret_type name signature																			\
{																														\
	ret_type ret;																							\
	void *handle;																							\
	ret_type (*fptr) signature;																\
	if (0&& print) {																							\
		printf("%d: %s\n", txc_xact_descriptor->tid, #name);		\
		txc_lockstat_print_cur_lockset();												\
	}																													\
	if (print) {																							\
		printf("Interpose>>>%d: %s\n", txc_xact_descriptor->tid, #name);		\
	}																													\
	handle = dlopen("/lib/libc.so.6", RTLD_LAZY);							\
  if(!handle) {																							\
    perror("dlopen");																				\
    exit(-1);																								\
  }																													\
  fptr = dlsym(handle, #name);															\
	ret = fptr args ;																					\
	return ret;																								\	
}

/*
ACTION(fdatasync, int, (int fd), (fd), 0);
ACTION(fsync, int, (int fd), (fd), 0);
ACTION(write, ssize_t, (int fd, const void *buf, size_t count), (fd, buf, count), 0);
ACTION(read, ssize_t, (int fd, void *buf, size_t count), (fd, buf, count), 0);
ACTION(close, int, (int fd), (fd), 1);
*/
