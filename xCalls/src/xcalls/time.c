#include <txc/int/transaction.h>
#include <txc/int/koa.h>
#include <txc/libc.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <txc/int/stat.h>
#include <txc/int/hrtime.h>
#include <txc/int/debug.h>
#include <sys/time.h>

TM_WAIVER
int 
txc_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	return gettimeofday(tv, tz);
}
