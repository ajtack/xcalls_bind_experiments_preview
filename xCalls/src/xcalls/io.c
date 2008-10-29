#include <txc/int/transaction.h>
#include <txc/int/koa.h>
#include <txc/libc.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <txc/int/stat.h>
#include <txc/int/hrtime.h>
#include <txc/int/debug.h>
#define GET_TIME 0

TM_WAIVER int __open(const char *pathname, int flags, mode_t mode);
TM_WAIVER ssize_t __read(int fd, void *buf, size_t count);
TM_WAIVER ssize_t __write(int fd, const void *buf, size_t count);
TM_WAIVER off_t __lseek(int fildes, off_t offset, int whence);
TM_WAIVER int __close(int fd);
TM_WAIVER int stat(const char *path, struct stat *buf); 

TM_WAIVER int printf(const char *format, ...);


/* Defer close() until commit */
txc_result_t
txc_fsync_commit(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];

	fsync(fildes);
}


TM_WAIVER 
int 
txc_fsync(int fildes) {
	int dummy;	
	txc_koa_t *koa;	
	int ret;
	txc_sentinel_t *sentinel;	
	unsigned int args_array[6];

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_SHARED(sentinel = koa->sentinel);
			ret = fsync(fildes);
			SENTINEL_RELEASE(sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			args_array[0] = (unsigned int) fildes;
			txc_register_commit_action(txc_fsync_commit, 1, args_array);
			ret = 0;
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = fsync(fildes);	
			break;
		default:
			assert(0);
	}

	return ret;
}


/* Defer close() until commit */
txc_result_t
txc_fdatasync_commit(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];

	fdatasync(fildes);
}


TM_WAIVER 
int 
txc_fdatasync(int fildes) {
	int dummy;	
	txc_koa_t *koa;	
	int ret;
	txc_sentinel_t *sentinel;	
	unsigned int args_array[6];

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_SHARED(sentinel = koa->sentinel);
			ret = fdatasync(fildes);
			SENTINEL_RELEASE(sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			args_array[0] = (unsigned int) fildes;
			txc_register_commit_action(txc_fdatasync_commit, 1, args_array);
			ret = 0;
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = fdatasync(fildes);	
			break;
		default:
			assert(0);
	}

	return ret;
}


/* int __open(const char *pathname, int flags, mode_t mode); */

TM_WAIVER void txc_open_compensation(unsigned int num_inputs, unsigned int* input_array);

#if 0
TM_WAIVER
int
__txc_open(const char *pathname, int flags, int speculative_open) {
    int fildes;
    unsigned int input_array[6];

	if(!speculative_open) {
		fildes = __open(pathname, flags);
		//printf("fildes: %d\n", fildes);
		return fildes;
	} else {
        fildes = __open(pathname, flags);
		if (fildes > 0) {
			input_array[0] = pathname;
    	    input_array[1] = flags;
			input_array[2] = fildes;
			txc_register_compensating_action(&txc_open_compensation, 3, input_array);
		}
		return fildes;
	}
}

TM_CALLABLE
int
txc_open(const char *pathname, int flags) {
	txc_koa_t *koa;	
	int fildes;
	int dummy;
	hrtime koa_create_begin, koa_create_end;
	printf("txc_open\n");	

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return __open(pathname, flags);	
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			fildes = __txc_open(pathname, flags, 0);
			//printf("open_irrevocable: fildes = %d\n", fildes);	
			if (fildes > 0) {
				if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes)
					 != TXC_R_SUCCESS) {
		    		assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, TXC_KOA_IS_FILE) == TXC_R_SUCCESS);
		    		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				} else {
			    	assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				}
			}
			return fildes;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			fildes = __txc_open(pathname, flags, 1);
			//printf("open_speculative: fildes = %d\n", fildes);	
#if GET_TIME
			koa_create_begin = gethrtime();
#endif
			if (fildes > 0) {
				if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
					!= TXC_R_SUCCESS) {
    				assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, TXC_KOA_IS_FILE) == TXC_R_SUCCESS);
		   			assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				} else {
			   		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				}
#if GET_TIME
			koa_create_end = gethrtime();
			printf("koa creation time: %lld us\n", ((koa_create_end-koa_create_begin)/1000));
#endif
				return fildes;
			} else {
				XACT_SWITCH_TO_IRREVOCABLE
			}
		default:
			assert(0);
	}
}
#endif

TM_WAIVER
int
__txc_open(const char *pathname, int flags, mode_t mode, int speculative_open) {
    int fildes;
    unsigned int input_array[6];

	if(!speculative_open) {
		fildes = __open(pathname, flags, mode);
		return fildes;
	} else {
		input_array[0] = (unsigned int) pathname;
    input_array[1] = flags;
    fildes = __open(pathname, flags, mode);
		input_array[2] = fildes;
		if (fildes > 0)
			txc_register_compensating_action(&txc_open_compensation, 3, input_array);
		return fildes;
	}
}

TM_CALLABLE
int
txc_open(const char *pathname, int flags, mode_t mode) {
	txc_koa_t *koa;	
	int fildes;
	int dummy;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			fildes = __txc_open(pathname, flags, mode, 0);
			if (fildes > 0) {
				if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
		    		assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, TXC_KOA_IS_FILE) == TXC_R_SUCCESS);
		    		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				} else {
			    	assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				}
			}
			return fildes;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			fildes = __txc_open(pathname, flags, mode, 1);
			if (fildes > 0) {
				if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
	    			assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, TXC_KOA_IS_FILE) == TXC_R_SUCCESS);
			   		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				} else {
			   		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				}
				return fildes;
			} else {
				XACT_SWITCH_TO_IRREVOCABLE
			}
		default:
			assert(0);
	}
}

/* Delete the file if the open() was invoked to create a file
 * Close the file otherwise
 */ 
TM_WAIVER
void
txc_open_compensation(unsigned int num_inputs, unsigned int* input_array)
{
	const char *path = (const char *) input_array[0];
	int isDel = (int) input_array[1];
	int fildes = (int) input_array[2];

	if (isDel & O_CREAT)
		unlink(path);
	else
		__close(fildes);

}

TM_WAIVER void txc_creat_compensation(unsigned int num_inputs, unsigned int* input_array);

TM_WAIVER
int 
txc_creat(const char *path, mode_t mode)
{
	txc_koa_t *koa;	
	int fildes;
	unsigned int input_array[6];

	
	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			fildes = creat(path, mode);
			if(fildes > 0) {
				if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
					!= TXC_R_SUCCESS) {
   		 			assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, TXC_KOA_IS_FILE) == TXC_R_SUCCESS);
			   		assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				} else {
		   			assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				}
			}
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			input_array[0] = path;
			fildes = creat(path, mode);
			if (fildes > 0) {
				if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
    				assert(txc_koa_create(txc_transactionmgr->koamgr, &koa, TXC_KOA_IS_FILE) == TXC_R_SUCCESS);
		   			assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				} else {
		   			assert(txc_koa_attach(txc_transactionmgr->koamgr, koa, fildes) == TXC_R_SUCCESS);
				}
				txc_register_compensating_action(&txc_creat_compensation, 3, input_array);
			}
			break;
	}
	return fildes;
}

/* Delete the file */ 
TM_WAIVER
void
txc_creat_compensation(unsigned int num_inputs, unsigned int* input_array)
{
	const char *path = (const char *) input_array[0];

	unlink(path);

}

TM_WAIVER void txc_close_commit(unsigned int num_inputs, unsigned int* input_array);

TM_WAIVER
int
__txc_close(int fildes, int speculative_close)
{
    unsigned int input_array[6];

	if(!speculative_close) {
		assert(txc_koa_detach(txc_transactionmgr->koamgr, fildes)
																										 == TXC_R_SUCCESS);
		return __close(fildes);
	} else {
    input_array[0] = fildes;
    txc_register_commit_action(&txc_close_commit, 1, input_array);
		return 0;
	}
}


TM_CALLABLE
int
txc_close(int fildes)
{
	int dummy;	
	txc_koa_t *koa;	
	int ret;
	txc_sentinel_t *sentinel;	

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_SHARED(sentinel = koa->sentinel);
			ret = __txc_close(fildes, 0);
			SENTINEL_RELEASE(sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			__txc_close(fildes, 1);
			ret = 0;
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																													!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_close(fildes, 0);	
			break;
		default:
			assert(0);
	}

	return ret;
}

/* Defer close() until commit */
TM_WAIVER
void
txc_close_commit(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];

	assert(txc_koa_detach(txc_transactionmgr->koamgr, fildes) == TXC_R_SUCCESS);
	__close(fildes);
}

TM_WAIVER void txc_unlink_commit(unsigned int num_inputs, unsigned int* input_array);
TM_WAIVER int unlink(const char *pathname);

//FIXME: we need to get the sentinel by querying the inode number
TM_CALLABLE
int 
txc_unlink(const char *path)
{
	int ret;
	unsigned int input_array[6];

	//printf("txc_unlink\n");
	ret = unlink(path);
	return ret;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			//FIXME	
			//SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = unlink(path);
			//SENTINEL_RELEASE(koa->sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			//SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = 0;
			input_array[0] = path;
			txc_register_commit_action(&txc_unlink_commit, 1, input_array);
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			//SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = unlink(path);
			break;
	}
	return ret;
}

/* Defer unlink() until commit */
TM_WAIVER
void
txc_unlink_commit(unsigned int num_inputs, unsigned int* input_array)
{
	const char *path = (const char *) input_array[0];

	unlink(path);
}

TM_WAIVER void txc_chmod_commit(unsigned int num_inputs, unsigned int* input_array);

TM_WAIVER
int 
txc_chmod(const char *path, mode_t mode)
{
  int ret;
	unsigned int input_array[6];
				
	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			ret = chmod(path, mode);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			ret = 0;
			input_array[0] = path;
			input_array[1] = mode;
			txc_register_commit_action(&txc_chmod_commit, 2, input_array);
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			ret = chmod(path, mode);
			break;
	}
	return ret;
}

/* Defer chmod() until commit */
TM_WAIVER
void
txc_chmod_commit(unsigned int num_inputs, unsigned int* input_array)
{
	const char *path = (const char *) input_array[0];
	mode_t mode = (mode_t) input_array[1];

	chmod(path, mode);
}

TM_WAIVER void txc_rename_commit(unsigned int num_inputs, unsigned int* input_array);
TM_WAIVER int rename(const char *oldpath, const char *newpath);

//FIXME: we need to get the sentinel by querying the inode number
TM_CALLABLE
int 
txc_rename(const char *old, const char *new)
{
	int ret;
	unsigned int input_array[6];
				
	//printf("txc_rename\n");
	//ret = rename(old, new);
	//return ret;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return rename(old, new);
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			ret = rename(old, new);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			printf("txc_rename: TXC_XACT_STATE_RUNNING_SPECULATIVE\n");
			return rename(old, new);
			//ret = 0;
			//input_array[0] = new;
			//input_array[1] = old;
			//txc_register_commit_action(&txc_rename_commit, 2, input_array);
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			ret = rename(old, new);
			break;
	}
	return ret;
}

/* Defer rename() until commit */
TM_WAIVER
void
txc_rename_commit(unsigned int num_inputs, unsigned int* input_array)
{
	const char *old = (const char *) input_array[0];
	const char *new = (const char *) input_array[1];

	rename(old, new);
}


TM_WAIVER void txc_read_compensation(unsigned int num_inputs, unsigned int* input_array);

TM_WAIVER
ssize_t
__txc_read(int fildes, void *buf, size_t nbyte, int speculative_read)
{
	int ret;
	unsigned int input_array[6];
#if GET_TIME
	hrtime bookkeeping_begin, bookkeeping_end, bookkeeping_total=0;
#endif

	if(!speculative_read) {
		return __read(fildes, buf, nbyte);
	} else {
		ret = __read(fildes, buf, nbyte);

#if GET_TIME
		bookkeeping_begin = gethrtime();
#endif
		if (ret > 0) {
			input_array[0] = fildes;
			input_array[1] = ret;
			txc_register_compensating_action(&txc_read_compensation, 2, input_array);
		}
#if GET_TIME
		bookkeeping_end = gethrtime();
		bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
		printf("txc_read: bookkeeping = %lld us\n", bookkeeping_total/1000);
#endif
		return ret;
	}
}

TM_CALLABLE
ssize_t
txc_read(int fildes, void *buf, size_t nbyte)
{
	txc_koa_t *koa;	
	int ret;
	int dummy;

#if GET_TIME
	hrtime sentinel_begin, sentinel_end, sentinel_total=0;
	hrtime koa_get_begin, koa_get_end;
#endif


//	txc_stat_action_incr("txc_read", __FILE__, __LINE__);
	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return __read(fildes, buf, nbyte);	
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																												!= TXC_R_SUCCESS) {
				TXC_INTERNALERROR("BAD koa read: fildes = %d\n", fildes);
			}
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __txc_read(fildes, buf, nbyte, 0);
			SENTINEL_RELEASE(koa->sentinel);
			return ret;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
#if GET_TIME
			koa_get_begin = gethrtime();
#endif
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 	
																												!= TXC_R_SUCCESS) {
				TXC_INTERNALERROR("BAD koa read: fildes = %d\n", fildes);
			}
#if GET_TIME
			koa_get_end = gethrtime();
			printf("txc_read: koa get = %lld us\n", (koa_get_end -koa_get_begin)/1000);
			sentinel_begin = gethrtime();
#endif
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
#if GET_TIME
			sentinel_end = gethrtime();
			printf("txc_read: acquire exclusive sentinel = %lld us\n", (sentinel_end-sentinel_begin)/1000);
#endif
			ret = __txc_read(fildes, buf, nbyte, 1);
			if (ret < 0) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				return ret;
			}
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
				TXC_INTERNALERROR("BAD koa read: fildes = %d\n", fildes);
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_read(fildes, buf, nbyte, 0);
			return ret;
		default:
			assert(0);
	}
}

/* Restore the original position */
TM_WAIVER
void
txc_read_compensation(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];
	int bytes_read = (int) input_array[1];

	__lseek(fildes, -bytes_read, SEEK_CUR);
}

/* No compensation required for pread() */
TM_WAIVER
ssize_t
__txc_pread(int fildes, void *buf, size_t nbyte, off_t offset)
{
	return pread(fildes, buf, nbyte, offset);
}


TM_CALLABLE
ssize_t
txc_pread(int fildes, void *buf, size_t nbyte, off_t offset)
{
	int dummy;	
	txc_koa_t *koa;	
	int ret;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return __txc_pread(fildes, buf, nbyte, offset);
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __txc_pread(fildes, buf, nbyte, offset);
			SENTINEL_RELEASE(koa->sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_pread(fildes, buf, nbyte, offset);
			if (ret < 0)
				XACT_SWITCH_TO_IRREVOCABLE
			//printf("txc_pread\n");
			break;
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:	
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_pread(fildes, buf, nbyte, offset);
			//printf("txc_pread\n");
			break;
	}
	return ret;
}

TM_WAIVER void txc_write_compensation_case1(unsigned int num_inputs, unsigned int* input_array);
TM_WAIVER void txc_write_compensation_case2(unsigned int num_inputs, unsigned int* input_array);
TM_WAIVER void txc_write_compensation_case31(unsigned int num_inputs, unsigned int* input_array);
TM_WAIVER void txc_write_compensation_case32(unsigned int num_inputs, unsigned int* input_array);


TM_WAIVER
ssize_t
__txc_write(int fildes, const void *buf, size_t nbyte, int flag, int speculative_write) {
	int ret;	
	unsigned int input_array[6];
	void *local_buf;

#if GET_TIME
	hrtime bookkeeping_begin, bookkeeping_end, bookkeeping_total=0;
#endif
	if (!speculative_write) {
		return __write(fildes, buf, nbyte);
	} else {
		if (fildes <= 2) {
		/* no compensation for stdin, stdout, stderr */
			ret = __write(fildes, buf, nbyte);
		} else {
			switch(flag) {
				case APPEND_ONLY:
					ret = __write(fildes, buf, nbyte);
#if GET_TIME
					bookkeeping_begin = gethrtime();
#endif
					if (ret > 0) {
						input_array[0] = fildes;
						input_array[1] = ret;	
						txc_register_compensating_action(&txc_write_compensation_case1, 2, input_array);
					}
#if GET_TIME
					bookkeeping_end = gethrtime();
					bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
					printf("txc_write: bookkeeping = %lld us\n", bookkeeping_total/1000);
#endif
					break;
				case OW_IGNORE:
					ret = __write(fildes, buf, nbyte);
#if GET_TIME
					bookkeeping_begin = gethrtime();	
#endif
					if (ret > 0) {
						input_array[0] = fildes;
						input_array[1] = ret;
						txc_register_compensating_action(&txc_write_compensation_case1, 2, input_array);
					}
#if GET_TIME
					bookkeeping_end = gethrtime();
					bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
					printf("txc_write: bookkeeping = %lld us\n", bookkeeping_total/1000);
#endif
					break;
				case OW_SAVE: 	
#if GET_TIME
					bookkeeping_begin = gethrtime();	
#endif
					input_array[0] = fildes;
					txc_transaction_log_buffer_allocate(nbyte*sizeof(char), &local_buf); //TODO: what if it fails?
					if ((ret = __read(fildes, local_buf, nbyte)) < nbyte) {
						input_array[1] = ret;
						__lseek(fildes, -ret, SEEK_CUR);
#if GET_TIME
						bookkeeping_end = gethrtime();
#endif
						ret = __write(fildes, buf, nbyte);
#if GET_TIME
						bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
						bookkeeping_begin = gethrtime();	
#endif
						if (ret > 0) {
							input_array[2] = (unsigned int) local_buf;
							input_array[3] = nbyte;
							txc_register_compensating_action(&txc_write_compensation_case31, 4, input_array);
						}
#if GET_TIME
						bookkeeping_end = gethrtime();
						bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
#endif
					} else {
						__lseek(fildes, -ret, SEEK_CUR);
#if GET_TIME
						bookkeeping_end = gethrtime();
#endif
						ret =__write(fildes, buf, nbyte);
#if GET_TIME
						bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
						bookkeeping_begin = gethrtime();	
#endif
						if (ret > 0) {
							input_array[2] = (unsigned int) local_buf;
							input_array[3] = nbyte;
							txc_register_compensating_action(&txc_write_compensation_case32, 4, input_array);
						}
#if GET_TIME
						bookkeeping_end = gethrtime();
						bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
#endif
					}
#if GET_TIME
					printf("txc_write: bookkeeping = %lld us\n", bookkeeping_total/1000);
#endif
					break;
				default:
					 //error //SENTINEL_RELEASE(koa->sentinel);//TODO: check??
			}
			return ret;
		}
	}
}

TM_CALLABLE
ssize_t
txc_write(int fildes, const void *buf, size_t nbyte, int flag)
{
	txc_koa_t *koa;	
	int ret;
	int dummy;
#if GET_TIME
	hrtime sentinel_begin, sentinel_end, sentinel_total=0;
	hrtime koa_get_begin, koa_get_end;
#endif

//	txc_stat_action_incr("txc_write", __FILE__, __LINE__);
	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
				assert(0);		
			}		
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __txc_write(fildes, buf, nbyte, flag, 0);
			SENTINEL_RELEASE(koa->sentinel);
			return ret;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
#if GET_TIME
			koa_get_begin = gethrtime();
#endif
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
				assert(0);		
			}
#if GET_TIME
			koa_get_end = gethrtime();
			printf("txc_write: koa get = %lld us\n", (koa_get_end - koa_get_begin)/1000);
			sentinel_begin = gethrtime();
#endif
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
#if GET_TIME
			sentinel_end = gethrtime();
			printf("txc_write: acquire exclusive sentinel = %lld us\n", (sentinel_end-sentinel_begin)/1000);
#endif
			ret  = __txc_write(fildes, buf, nbyte, flag, 1);
			if (0 > ret) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				//printf("txc_write\n");
				return ret;
			}
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			return __txc_write(fildes, buf, nbyte, flag, 0);
		default:
			assert(0);
	}

}

/* Append Only - truncate */
TM_WAIVER
void
txc_write_compensation_case1(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];
	int bytes_written = (int) input_array[1];
	ftruncate(fildes, __lseek(fildes, -bytes_written, SEEK_CUR));
}

/* Overwrite and ignore - truncate and restore the original position */
TM_WAIVER
void
txc_write_compensation_case2(unsigned int num_inputs, unsigned int* input_array)
{
    int fildes = (int) input_array[0];
    off_t initial_file_size  = (off_t) input_array[1];
	off_t old_position = (off_t) input_array[2];
    ftruncate(fildes, initial_file_size);
	__lseek(fildes, old_position, SEEK_SET);
}

/* Overwrite and save (wrote more data than already present) - 
 * restore old contents, old position and truncate 
 */
TM_WAIVER
void
txc_write_compensation_case31(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];
	void* local_buf = (void*) input_array[2];
	int nbyte = (int) input_array[3];
	int bytes_overwritten = input_array[1];

	ftruncate(fildes, __lseek(fildes, -nbyte, SEEK_CUR));
	__write(fildes, local_buf, bytes_overwritten);
	__lseek(fildes, -bytes_overwritten, SEEK_CUR);
}

/* Overwrite and save (wrote less data than already present) - 
 * restore old contents, old position 
 */
TM_WAIVER
void
txc_write_compensation_case32(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];
	void* local_buf = (void*) input_array[2];
	int nbyte = (int) input_array[3];
	int for_old = (int) input_array[1];

	__lseek(fildes, -nbyte, SEEK_SET);
	__write(fildes, local_buf, for_old);
	__lseek(fildes, -for_old, SEEK_CUR);
}


TM_WAIVER void txc_pwrite_compensation_case1(unsigned int num_inputs, unsigned int* input_array);
TM_WAIVER void txc_pwrite_compensation_case2(unsigned int num_inputs, unsigned int* input_array);

/* Common case overwrite and save - for append only txc_write() is more suitable */
TM_WAIVER
ssize_t
__txc_pwrite(int fildes, const void *buf, size_t nbyte, off_t offset, int speculative_pwrite)
{
	int ret;
	unsigned int input_array[6];
	void *local_buf;

	if(!speculative_pwrite) {
		return pwrite(fildes, buf, nbyte, offset);
	} else {
		txc_transaction_log_buffer_allocate(nbyte*sizeof(char), &local_buf); //TODO: what if this fails?
		if ((ret = pread(fildes, local_buf, nbyte, offset)) < nbyte) {
			input_array[4] = ret;
			ret = pwrite(fildes, buf, nbyte, offset);
			if (ret > 0) {
				input_array[0] = fildes;
				input_array[1] = offset;
				input_array[2] = (unsigned int) local_buf;
				input_array[3] = nbyte;
				txc_register_compensating_action(&txc_pwrite_compensation_case1, 5, input_array);
			}
		} else {
			ret = pwrite(fildes, buf, nbyte, offset);
			if (ret > 0) {
				//printf("txc_pwrite\n");
				input_array[2] = (unsigned int) local_buf;
				input_array[3] = nbyte;
				txc_register_compensating_action(&txc_pwrite_compensation_case2, 4, input_array);
			}
		}
		return ret;
	}
}

TM_CALLABLE
ssize_t
txc_pwrite(int fildes, const void *buf, size_t nbyte, off_t offset)
{
	int dummy;	
	txc_koa_t *koa;	
	int ret;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return __txc_pwrite(fildes, buf, nbyte, offset, 0);
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __txc_pwrite(fildes, buf, nbyte, offset, 0);
			SENTINEL_RELEASE(koa->sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_pwrite(fildes, buf, nbyte, offset, 1);
			if (0 > ret)
			break;	
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __txc_pwrite(fildes, buf, nbyte, offset, 0);
			break;
	}
	return ret;
}

/* Restore the original contents and truncate if 
 * more data is written than already present
 */ 
TM_WAIVER
void
txc_pwrite_compensation_case1(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];
	off_t offset = (off_t) input_array[1];
	void* local_buf = (void*) input_array[2];
	int nbyte = (int) input_array[3];
	//off_t initial_file_size  = (off_t) input_array[4];
	int bytes_overwritten = (int) input_array[4];

	//pwrite(fildes, local_buf, nbyte, offset) ;
	pwrite(fildes, local_buf, bytes_overwritten, offset) ;
	//ftruncate(fildes, initial_file_size);
	ftruncate(fildes, offset+bytes_overwritten);
}

/* Restore the original contents */
TM_WAIVER
void
txc_pwrite_compensation_case2(unsigned int num_inputs, unsigned int* input_array)
{
	int fildes = (int) input_array[0];
	off_t offset = (off_t) input_array[1];
	void* local_buf = (void*) input_array[2];
	unsigned int nbyte = input_array[3];

	pwrite((int) fildes, (void *) local_buf, nbyte, offset) ;
}

TM_WAIVER void txc_lseek64_compensation_case(unsigned int num_inputs, unsigned int* input_array);

TM_WAIVER
off64_t
__txc_lseek64(int fildes, off64_t offset, int whence, int speculative_lseek)
{
	unsigned int input_array[6];
	off_t ret;

	if (!speculative_lseek) {
		return lseek64(fildes, offset, whence);
	} else {
   	input_array[0] = fildes;
		input_array[1] = (unsigned int ) lseek64(fildes, 0, SEEK_CUR);
   	txc_register_compensating_action(&txc_lseek64_compensation_case, 2, input_array);
   	return lseek64(fildes, offset, whence);
	}
}


TM_WAIVER
off64_t
txc_lseek64(int fildes, off64_t offset, int whence)
{
	txc_koa_t *koa;	
	int dummy;
	int ret;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return lseek64(fildes, 0, SEEK_CUR);
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
    		ret = __txc_lseek64(fildes, offset, whence, 0);
			SENTINEL_RELEASE(koa->sentinel);
			return ret;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
   		ret = __txc_lseek64(fildes, offset, whence, 1);
			if (0 > ret) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				return ret;
			}
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			return __txc_lseek64(fildes, offset, whence, 0);
		default:
			assert(0);
	}
}


/* Seek to the original position */
TM_WAIVER
void
txc_lseek64_compensation_case(unsigned int num_inputs, unsigned int* input_array)
{ 
	int fildes = (int) input_array[0];
	off64_t old_position = (off64_t) input_array[1];
 	lseek64(fildes, old_position, SEEK_SET);
}



TM_WAIVER void txc_lseek_compensation_case(unsigned int num_inputs, unsigned int* input_array);

TM_WAIVER
off_t
__txc_lseek(int fildes, off_t offset, int whence, int speculative_lseek)
{
	unsigned int input_array[6];
	off_t ret;

#if GET_TIME
	hrtime bookkeeping_begin, bookkeeping_end, bookkeeping_total=0;
#endif

	if (!speculative_lseek) {
		return __lseek(fildes, offset, whence);
	} else {
#if GET_TIME
		bookkeeping_begin = gethrtime();	
#endif
      	input_array[0] = fildes;
		input_array[1] = __lseek(fildes, 0, SEEK_CUR);
      	txc_register_compensating_action(&txc_lseek_compensation_case, 2, input_array);
#if GET_TIME
		bookkeeping_end = gethrtime();
		bookkeeping_total = bookkeeping_total + bookkeeping_end - bookkeeping_begin;
		printf("txc_lseek: bookkeeping = %lld us\n", bookkeeping_total/1000);
#endif
      	return __lseek(fildes, offset, whence);
	}
}


TM_CALLABLE
off_t
txc_lseek(int fildes, off_t offset, int whence)
{
	txc_koa_t *koa;	
	int dummy;
	int ret;

#if GET_TIME
	hrtime sentinel_begin, sentinel_end, sentinel_total=0;
#endif

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NOT_INIT:
			return __lseek(fildes, 0, SEEK_CUR);
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
    		ret = __txc_lseek(fildes, offset, whence, 0);
			SENTINEL_RELEASE(koa->sentinel);
			return ret;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
#if GET_TIME
			sentinel_begin = gethrtime();
#endif
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
#if GET_TIME
			sentinel_end = gethrtime();
			printf("txc_lseek: acquire exclusive sentinel = %lld us\n", (sentinel_end-sentinel_begin)/1000);
#endif
      		ret = __txc_lseek(fildes, offset, whence, 1);
			if (0 > ret) {
				XACT_SWITCH_TO_IRREVOCABLE
			} else {
				//printf("txc_lseek\n");
				return ret;
			}
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			return __txc_lseek(fildes, offset, whence, 0);
		default:
			assert(0);
	}
}

/* Seek to the original position */
TM_WAIVER
void
txc_lseek_compensation_case(unsigned int num_inputs, unsigned int* input_array)
{ 
	int fildes = (int) input_array[0];
	off_t old_position = (off_t) input_array[1];
    __lseek(fildes, old_position, SEEK_SET);
}

TM_WAIVER
int
txc_stat(const char *path, struct stat *buf) 
{
	return stat(path, buf);
}

TM_WAIVER
int
txc_fcntl(int fildes, int cmd, long arg) {
	txc_koa_t *koa;
	int ret;
	int mode;

	switch(mode = txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) != TXC_R_SUCCESS) {
				assert(0);		
			}		
			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __fcntl(fildes, cmd, arg);
			SENTINEL_RELEASE(koa->sentinel);
			return ret;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			XACT_SWITCH_TO_IRREVOCABLE
			/* pass through if change to irrevocable succeeds, otherwise the transaction 
			 * restarts and we don't get here */ 
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			if (txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) 
																												!= TXC_R_SUCCESS) {
				assert(0);		
			}
			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
			ret = __fcntl(fildes, cmd, arg);
			switch(cmd) {
				case F_DUPFD:
					//TODO
					/* A new file descriptor is allocated -- need to protect it using 
					 * the same sentinel */
					assert(0);
					break;
				default:
			}
			break;
		default:
			TXC_INTERNALERROR("Unknown transaction mode: %d\n", mode);
	}
	return ret;
}


TM_WAIVER
int
__txc_mkdir(const char *pathname, mode_t mode)
{
	return mkdir(pathname, mode);
}

TM_CALLABLE
int
txc_mkdir(const char *pathname, mode_t mode)
{
	txc_koa_t *koa;
	int ret;

	switch(txc_transaction_xact_state_get()) {
		case TXC_XACT_STATE_NON_TRANSACTIONAL:
//			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
//			SENTINEL_ACQUIRE_SHARED(koa->sentinel);
			ret = __txc_mkdir(pathname, mode);
//			SENTINEL_RELEASE(koa->sentinel);
			break;
		case TXC_XACT_STATE_RUNNING_SPECULATIVE:
			abort();
//			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
//			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
//			ret = __txc_pwrite(fildes, buf, nbyte, offset, 1);
			break;	
		case TXC_XACT_STATE_RUNNING_IRREVOCABLE:
			abort();
//			assert(txc_koa_get(txc_transactionmgr->koamgr, &koa, fildes) == TXC_R_SUCCESS);
//			SENTINEL_ACQUIRE_EXCLUSIVE(koa->sentinel);
//			ret = __txc_pwrite(fildes, buf, nbyte, offset, 0);
			break;
	}

	return ret;
}



