#include <txc/int/transaction.h>
#include <txc/int/koa.h>
#include <sys/types.h>
#include <unistd.h>


TM_WAIVER
pid_t 
txc_getpid(void)
{
	return getpid();
}


TM_WAIVER
pid_t 
txc_getppid(void)
{
	return getppid();
}
