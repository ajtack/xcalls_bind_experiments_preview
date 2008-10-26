#ifndef  _TXC_HRTIME_H
#define _TXC_HRTIME_H

#define hrtime   unsigned long long

TM_WAIVER
static inline 
hrtime gethrtime() {
	hrtime t;
	asm volatile( "rdtsc" : "=A" (t));
	return t;
}

#endif
