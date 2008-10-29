#include <txc/int/transaction.h>
#include <txc/int/debug.h>
#include <txc/int/stat.h>

/* 
 * Intel STM v2.0 already provides support for memory allocation inside
 * transactions. Here we provide wrappers to these functions. The reason
 * doing this is to be able to use the -fno-builtin option with the ICC
 * compiler to prevent the use of intrinsic functions. In the past we 
 * have seen problems with the compiler not generating a warning message 
 * when inlining instrinsic functions inside a transaction. This is 
 * dangerous and we want to avoid that. However by using the -fno-builtin
 * option this implies the compiler does not use the transactional 
 * allocation routines. That is why we provide these wrappers.
 */ 

TM_CALLABLE
void 
*txc_malloc(size_t size) 
{
	return malloc(size); 
}

TM_CALLABLE
void 
*txc_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

TM_CALLABLE
void
txc_free(void *ptr)
{
	free(ptr);
}

TM_CALLABLE
void 
*txc_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

