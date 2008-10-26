int
txc_atomic_xadd(int *p, int val) {
	int prev = val;
	__asm__ volatile(
		"lock;"
		"xadd %0, %1"
		:"=q"(prev)
		:"m"(*p), "0"(prev)
		:"memory", "cc");
	return (prev);
}

void
txc_atomic_store(int *p, int val) {
	__asm__ volatile(
		"lock;"		
		"xchgl %1, %0"
		:
		: "r"(val), "m"(*p)
		: "memory");
}

int
txc_atomic_cmpxchg(int *p, int cmpval, int val) {
	__asm__ volatile(
		"lock;"
		"cmpxchgl %1, %2"
		: "=a"(cmpval)
		: "r"(val), "m"(*p), "a"(cmpval)
		: "memory");

	return (cmpval);
}
