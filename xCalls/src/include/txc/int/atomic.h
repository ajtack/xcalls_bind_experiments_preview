#ifndef _TXC_ATOMIC_H
#define _TXC_ATOMIC_H

#include <sys/types.h>

int txc_atomic_xadd(int *p, int val);
void txc_atomic_store(int *p, int val);
int txc_atomic_cmpxchg(int *p, int cmpval, int val);

#endif
