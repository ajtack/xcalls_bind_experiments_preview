/*************************************************************************
 * Provider: LOCKSTAT
 *************************************************************************/


#define TXC_LOCKSTAT_MAX_LOCKERS						32		
#define TXC_LOCKSTAT_MAX_LOCKSET_SIZE				1024		

//#define KEEP_LOCKSET

/* LOCKSTAT data */

typedef struct txc_lockstat_locker_s txc_lockstat_locker_t;
typedef struct txc_lockstat_lockset_entry_s txc_lockstat_lockset_entry_t;

struct txc_lockstat_locker_s {
  unsigned long long count;
	char *file;
	int line;
  struct timeval    locked_total;
  struct timeval    wait_total;
};

struct txc_lockstat_lockset_entry_s {
	char *locker_file;
	char *locker_line;
	txc_stat_eventset_t *eventset;
};
 
PROVIDER_GLOBAL_DATA(LOCKSTAT)
{
};

PROVIDER_DATA(LOCKSTAT)
{
	unsigned int lockset_size;
	txc_lockstat_lockset_entry_t lockset[TXC_LOCKSTAT_MAX_LOCKSET_SIZE];
};

EVENTSET_DATA(LOCKSTAT)
{
	struct timeval ts;
	txc_lockstat_locker_t lockers[TXC_LOCKSTAT_MAX_LOCKERS];
	unsigned int num_lockers;
};

#define LOCKSTAT_EVENT_DATA																		\
	unsigned long long count;																		

EVENT_DATA(LOCKSTAT, SPINWAIT)
{
	LOCKSTAT_EVENT_DATA
};

EVENT_DATA(LOCKSTAT, BLOCKWAIT)
{
	LOCKSTAT_EVENT_DATA
};

EVENT_DATA(LOCKSTAT, ACQUIRE)
{
	LOCKSTAT_EVENT_DATA
	struct timeval wait_total;
};

EVENT_DATA(LOCKSTAT, RELEASE)
{
	LOCKSTAT_EVENT_DATA
};


/* LOCKSTAT initializers */

PROVIDER_GLOBAL_INIT(LOCKSTAT)
{
}

PROVIDER_INIT(LOCKSTAT)
{
#ifdef KEEP_LOCKSET
	ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset_size = 0;
	for (int i=0; i<TXC_LOCKSTAT_MAX_LOCKSET_SIZE; i++) {
		ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset = 0x0;
		ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_file = NULL;
		ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_line = 0x0;
	}
#endif
}

EVENTSET_INIT(LOCKSTAT)
{
	timevalclear(&(ACCESS_EVENTSET_DATA(LOCKSTAT)->ts));
	for (int i = 0; i < TXC_LOCKSTAT_MAX_LOCKERS; i++) {
		ACCESS_EVENTSET_DATA(LOCKSTAT)->lockers[i].file = NULL;
    ACCESS_EVENTSET_DATA(LOCKSTAT)->lockers[i].line = 0;
    ACCESS_EVENTSET_DATA(LOCKSTAT)->lockers[i].count = 0;
    timevalclear(&(ACCESS_EVENTSET_DATA(LOCKSTAT)->lockers[i].locked_total));
    timevalclear(&(ACCESS_EVENTSET_DATA(LOCKSTAT)->lockers[i].wait_total));
  }
}

#define LOCKSTAT_EVENT_INIT(event)						\
	ACCESS_EVENT_DATA(LOCKSTAT, event)->count = 0;

EVENT_INIT(LOCKSTAT, SPINWAIT) 
{	
	LOCKSTAT_EVENT_INIT(SPINWAIT) 
}

EVENT_INIT(LOCKSTAT, BLOCKWAIT) 
{	
	LOCKSTAT_EVENT_INIT(BLOCKWAIT) 
}

EVENT_INIT(LOCKSTAT, ACQUIRE) 
{	
	LOCKSTAT_EVENT_INIT(ACQUIRE) 
}

EVENT_INIT(LOCKSTAT, RELEASE) 
{	
	LOCKSTAT_EVENT_INIT(RELEASE) 
}



/* LOCKSTAT probes */

EVENT_PROBE(LOCKSTAT, SPINWAIT) 
{
	gettimeofday(&(ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->ts), NULL);
	ACCESS_EVENT_DATA(LOCKSTAT, SPINWAIT)->count++;
}

EVENT_PROBE(LOCKSTAT, BLOCKWAIT) 
{
	ACCESS_EVENT_DATA(LOCKSTAT, BLOCKWAIT)->count++;
}

EVENT_PROBE(LOCKSTAT, ACQUIRE) 
{
	txc_src_location_t *src_location = (txc_src_location_t *) arg;
	txc_lockstat_locker_t	*locker;
	struct timeval wait;

	txc_assert(src_location->file != NULL);

	ACCESS_EVENT_DATA(LOCKSTAT, ACQUIRE)->count++;

	/* Compute waiting time just for contended locks. Contended locks must
   * have invoked SPINWAIT probe before getting here. In such a case the 
   * timestamp (ts) is not zero */ 
	if (ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->ts.tv_sec != 0
				&& ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->ts.tv_usec != 0) {
		gettimeofday(&wait, NULL);
		timevalsub(&wait,	&(ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->ts));
		timevaladd(&(ACCESS_EVENT_DATA(LOCKSTAT, ACQUIRE)->wait_total), &wait);
		timevalclear(&(ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->ts));
		locker = NULL;
		for (int i=0; i < TXC_LOCKSTAT_MAX_LOCKERS; i++) {
			if (ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].file == NULL) {
				locker = &(ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i]);
				locker->file = src_location->file;
				locker->line = src_location->line;
				break;
			} else if (ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].file 
										== src_location->file && 
								 ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].line 
										== src_location->line) {
				locker = &(ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i]);
				break;
			} 
		}
		if (locker != NULL) {
			timevaladd(&(locker->wait_total), &wait);
			locker->count++;
		}
	}

#ifdef KEEP_LOCKSET
	int lockset_size = ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset_size; 
	//printf("T%d: ACQUIRE[%d](%p) %s(%d)\n", txc_xact_descriptor->tid, lockset_size, this->eventset, src_location->file, src_location->line);
	if (src_location->line == 0) {
		return;
	} 
	int overflow = 1;
	for (int i=0; i<TXC_LOCKSTAT_MAX_LOCKSET_SIZE; i++) {	
		if (ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset == 0x0) {

			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_file = 
																											src_location->file; 
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_line = 
																											src_location->line; 
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset = 
																											this->eventset;
			overflow = 0; 
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset_size++; 
			break;
		}
	}
	if (overflow) {
		txc_lockstat_print_cur_lockset();
		TXC_INTERNALERROR("Lock set size overflow: %d\n", lockset_size); 
	}
#endif
}

EVENT_PROBE(LOCKSTAT, RELEASE) 
{
	txc_src_location_t *src_location = (txc_src_location_t *) arg;

	ACCESS_EVENT_DATA(LOCKSTAT, RELEASE)->count++;
#ifdef KEEP_LOCKSET
	int lockset_size = ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset_size; 
//	printf("T%d: RELEASE[%d](%p) %s(%d)\n", txc_xact_descriptor->tid, lockset_size-1, this->eventset, src_location->file, src_location->line);
	if (src_location->line == 0) {
		return;
	} 
	int found = 0;
	for (int i=0; i<TXC_LOCKSTAT_MAX_LOCKSET_SIZE; i++) {	
		if (ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset == this->eventset) {
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_file = NULL;
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_line = 0x0;
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset = 0x0;
			found = 1; 
			ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset_size--; 
			break;
		}
	}
	if (found==0) {
		printf("T%d: RELEASE[%d](%p) %s(%d)\n", txc_xact_descriptor->tid, lockset_size-1, this->eventset, src_location->file, src_location->line);
		printf("T:%d: LOCK NOT FOUND IN MY SET\n", txc_xact_descriptor->tid);
		//txc_lockstat_print_cur_lockset();
		//abort(); 
	}	
#endif
}

/* LOCKSTAT Reports */

EVENT_REPORT(LOCKSTAT, SPINWAIT)
{
	stat_printf("\t\tSPINWAIT\n");
	stat_printf("\t\t\tcount = %lld\n",
								ACCESS_EVENT_DATA(LOCKSTAT, SPINWAIT)->count);
}

EVENT_REPORT(LOCKSTAT, BLOCKWAIT)
{
	stat_printf("\t\tBLOCKWAIT\n");
	stat_printf("\t\t\tcount = %lld\n",
								ACCESS_EVENT_DATA(LOCKSTAT, BLOCKWAIT)->count);
}

EVENT_REPORT(LOCKSTAT, ACQUIRE)
{
	stat_printf("\t\tACQUIRE\n");
	stat_printf("\t\t\tcount = %lld\n",
							ACCESS_EVENT_DATA(LOCKSTAT, ACQUIRE)->count);
	stat_printf("\t\t\twait_total = %d sec, %d usec\n",
							ACCESS_EVENT_DATA(LOCKSTAT, ACQUIRE)->wait_total.tv_sec,
							ACCESS_EVENT_DATA(LOCKSTAT, ACQUIRE)->wait_total.tv_usec);
	for (int i=0; i<TXC_LOCKSTAT_MAX_LOCKERS; i++) {
		if (ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].file == NULL) {
			break;
		}
		stat_printf("\t\t\tLocker %s(%d): [Count = %lld, Wait = %d sec, %d usec]\n",
							ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].file,							
							ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].line,
							ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].count,
							ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].wait_total.tv_sec,
							ACCESS_EVENT_EVENTSET_DATA(LOCKSTAT)->lockers[i].wait_total.tv_usec);
	}	
}

EVENT_REPORT(LOCKSTAT, RELEASE)
{

}


void
txc_lockstat_print_cur_lockset() {
#ifdef KEEP_LOCKSET
	int i;
	int setsize = 0;
	printf("T%d: Size of lock set: %d\n", txc_xact_descriptor->tid, ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset_size);
	for (int i=0; i<TXC_LOCKSTAT_MAX_LOCKSET_SIZE; i++) {	
		if (ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset != 0) {
			printf("\t[%d]: %p %s(%d)\n", ++setsize,
							ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].eventset,
							ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_file,
							ACCESS_PROVIDER_DATA(LOCKSTAT)->lockset[i].locker_line);
		}
	}
#endif
}



/*************************************************************************
 * Provider: SYNCSTAT
 *************************************************************************/

/* SYNCSTAT Global data shared by all threads */

PROVIDER_GLOBAL_DATA(SYNCSTAT)
{
	unsigned int glock;
};


/* SYNCSTAT per thread data */

PROVIDER_DATA(SYNCSTAT)
{
};

EVENTSET_DATA(SYNCSTAT)
{
};

EVENT_DATA(SYNCSTAT, ACQUIRE_GLOCK)
{
	unsigned long long count;
};

EVENT_DATA(SYNCSTAT, RELEASE_GLOCK)
{
};

/* SYNCSTAT initializers */

PROVIDER_GLOBAL_INIT(SYNCSTAT)
{
	ACCESS_PROVIDER_GLOBAL_DATA(SYNCSTAT)->glock = 0;
}

PROVIDER_INIT(SYNCSTAT)
{
}

EVENTSET_INIT(SYNCSTAT)
{
}

EVENT_INIT(SYNCSTAT, ACQUIRE_GLOCK) 
{	
	ACCESS_EVENT_DATA(SYNCSTAT, ACQUIRE_GLOCK)->count = 0;
}

EVENT_INIT(SYNCSTAT, RELEASE_GLOCK) 
{	
}


/* SYNCSTAT probes */

EVENT_PROBE(SYNCSTAT, ACQUIRE_GLOCK) 
{
	ACCESS_PROVIDER_GLOBAL_DATA(SYNCSTAT)->glock = 1;
	ACCESS_EVENT_DATA(SYNCSTAT, ACQUIRE_GLOCK)->count++;
}

EVENT_PROBE(SYNCSTAT, RELEASE_GLOCK) 
{
	ACCESS_PROVIDER_GLOBAL_DATA(SYNCSTAT)->glock = 0;
}


/* SYNCSTAT Reports */

EVENT_REPORT(SYNCSTAT, ACQUIRE_GLOCK)
{
	stat_printf("\t\t%-15s\t[ count = %lld ]\n", 
								"ACQUIRE_GLOCK",
								ACCESS_EVENT_DATA(SYNCSTAT, ACQUIRE_GLOCK)->count);
}

EVENT_REPORT(SYNCSTAT, RELEASE_GLOCK)
{
}



/*************************************************************************
 * Provider: XACTSTAT
 *************************************************************************/

/* XACTSTAT Global data shared by all threads */

PROVIDER_GLOBAL_DATA(XACTSTAT)
{
};


/* XACTSTAT per thread data */

PROVIDER_DATA(XACTSTAT)
{
};

EVENTSET_DATA(XACTSTAT)
{
	unsigned long long count;
	struct timeval begin_ts;
	struct timeval duration_total;
};

EVENT_DATA(XACTSTAT, XCALL)
{
	unsigned long long count;
};

EVENT_DATA(XACTSTAT, WOULDWAIT)
{
	unsigned long long count;
};

EVENT_DATA(XACTSTAT, BEGIN)
{
	unsigned long long count;
};

EVENT_DATA(XACTSTAT, END)
{
	unsigned long long count;
};


/* XACTSTAT initializers */

PROVIDER_GLOBAL_INIT(XACTSTAT)
{
}

PROVIDER_INIT(XACTSTAT)
{
}

EVENTSET_INIT(XACTSTAT)
{
	timevalclear(&(ACCESS_EVENTSET_DATA(XACTSTAT)->begin_ts));
	timevalclear(&(ACCESS_EVENTSET_DATA(XACTSTAT)->duration_total));
}

EVENT_INIT(XACTSTAT, XCALL) 
{	
	ACCESS_EVENT_DATA(XACTSTAT, XCALL)->count = 0;
}

EVENT_INIT(XACTSTAT, WOULDWAIT) 
{	
	ACCESS_EVENT_DATA(XACTSTAT, WOULDWAIT)->count = 0;
}

EVENT_INIT(XACTSTAT, BEGIN) 
{	
	ACCESS_EVENT_DATA(XACTSTAT, BEGIN)->count = 0;
}

EVENT_INIT(XACTSTAT, END) 
{	
	ACCESS_EVENT_DATA(XACTSTAT, END)->count = 0;
}



/* XACTSTAT probes */

EVENT_PROBE(XACTSTAT, XCALL) 
{
	ACCESS_EVENT_DATA(XACTSTAT, XCALL)->count++;
}

EVENT_PROBE(XACTSTAT, WOULDWAIT) 
{
	if (ACCESS_PROVIDER_GLOBAL_DATA(SYNCSTAT)->glock > 0) {
		ACCESS_EVENT_DATA(XACTSTAT, WOULDWAIT)->count++;
	}
}

EVENT_PROBE(XACTSTAT, BEGIN) 
{
	gettimeofday(&(ACCESS_EVENT_EVENTSET_DATA(XACTSTAT)->begin_ts), NULL);
	ACCESS_EVENT_DATA(XACTSTAT, BEGIN)->count++;
}

EVENT_PROBE(XACTSTAT, END) 
{
	struct timeval duration;
	gettimeofday(&duration, NULL);
	timevalsub(&duration,	&(ACCESS_EVENT_EVENTSET_DATA(XACTSTAT)->begin_ts));
	timevaladd(&(ACCESS_EVENT_EVENTSET_DATA(XACTSTAT)->duration_total), 
																																&duration);
	ACCESS_EVENT_DATA(XACTSTAT, END)->count++;
}


/* XACTSTAT Reports */

EVENT_REPORT(XACTSTAT, XCALL)
{
	stat_printf("\t\t%-10s\t[ count = %lld ]\n", 
								"XCALL",
								ACCESS_EVENT_DATA(XACTSTAT, XCALL)->count);
}

EVENT_REPORT(XACTSTAT, WOULDWAIT)
{
	stat_printf("\t\t%-10s\t[ count = %lld ]\n", 
								"WOULDWAIT",
								ACCESS_EVENT_DATA(XACTSTAT, WOULDWAIT)->count);
}

EVENT_REPORT(XACTSTAT, BEGIN)
{
}

EVENT_REPORT(XACTSTAT, END)
{
	stat_printf("\t\t%-10s\t[ count = %lld, duration_total = %d sec %d usec ]\n", 
								"Transactions",
								ACCESS_EVENT_DATA(XACTSTAT, WOULDWAIT)->count,
								ACCESS_EVENT_EVENTSET_DATA(XACTSTAT)->duration_total.tv_sec,
								ACCESS_EVENT_EVENTSET_DATA(XACTSTAT)->duration_total.tv_usec);
}
