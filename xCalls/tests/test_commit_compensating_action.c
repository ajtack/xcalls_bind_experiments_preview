#include <txc.h>
#include <math.h>
#include <assert.h>
#include "test_util.h"

#ifndef TXC_XCALLS_ENABLE
	#error /* Must build with xCalls support */
#endif

/*
 * To capture the sequence of operations we use Godel's encoding.    
 */

char *test_name = "test_commit_compensating_action";
unsigned int seq_num = 0;	
int retries;
unsigned int test1_compensating_action_1_seq;
unsigned int test1_compensating_action_2_seq;
unsigned long long score_commit; 
unsigned long long score_compensation; 
unsigned long long score_compensation2; 
unsigned int godel[] = {2,3,5,7,11,13,17,23,31}; 

void 
test1_commit_action_1(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 0); 
	score_commit *= pow(godel[0], ++seq_num);
}

void 
test1_commit_action_2(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 0); 
	score_commit *= pow(godel[1], ++seq_num);
}


void 
test1_compensating_action_1(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 0); 
	if (test1_compensating_action_1_seq++ == 0) {
		score_compensation *= pow(godel[2], ++seq_num);
//		printf("action1: score_compensation = %lld\n", score_compensation);
	} else {
		score_compensation2 *= pow(godel[2], ++seq_num);
//		printf("action1: score_compensation2 = %lld\n", score_compensation2);
	}
}

void 
test1_compensating_action_2(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 0); 
	if (test1_compensating_action_2_seq++ == 0) {
		score_compensation *= pow(godel[3], ++seq_num);
//		printf("action2: score_compensation = %lld\n", score_compensation);
	} else {
		score_compensation2 *= pow(godel[3], ++seq_num);
//		printf("action2: score_compensation2 = %lld\n", score_compensation2);
	}
}

int 
test1() 
{
	int i;
	int stress = 5;

	for (i=0; i<stress; i++) {
		retries = 0;
		seq_num = 0;
		score_compensation = 1;
		test1_compensating_action_1_seq=0;
		test1_compensating_action_2_seq=0;
		score_commit = 1;
	
		XACT_BEGIN(xact_1)
			__tm_waiver { seq_num++;	}
			XACT_BEGIN(xact_2)
				txc_register_commit_action(test1_commit_action_1, 0, NULL);
				txc_register_compensating_action(test1_compensating_action_1, 0, NULL);
			XACT_END(xact_2)
			txc_register_commit_action(test1_commit_action_2, 0, NULL);
			txc_register_compensating_action(test1_compensating_action_2, 0, NULL);
			__tm_waiver {
				if (retries++ == 0) {
					XACT_RETRY
				}
			}
		XACT_END(xact_1)
		if (score_commit == 23328 && score_compensation == 6125) {
			continue;
		} else {
			SUBTEST_PRINT_RESULT(1, TEST_FAILURE);
			return 1;
		}
	}
	SUBTEST_PRINT_RESULT(1, TEST_SUCCESS);
	return 0;
}

int 
test2() 
{
	int i;
	int stress = 5;

	for (i=0; i<stress; i++) {
		retries = 0;
		seq_num = 0;
		score_compensation = 1;
		score_compensation2 = 1;
		test1_compensating_action_1_seq=0;
		test1_compensating_action_2_seq=0;
		score_commit = 1;
	
		XACT_BEGIN(xact_1)
			__tm_waiver { seq_num++;	}
			XACT_BEGIN(xact_2)
				txc_register_commit_action(test1_commit_action_1, 0, NULL);
				txc_register_compensating_action(test1_compensating_action_1, 0, NULL);
			XACT_END(xact_2)
			txc_register_commit_action(test1_commit_action_2, 0, NULL);
			txc_register_compensating_action(test1_compensating_action_2, 0, NULL);
			__tm_waiver {
				if (retries++ < 2) {
					XACT_RETRY
				}
			}
		XACT_END(xact_1)
		if (score_commit == 5038848 && score_compensation == 6125 && score_compensation2 == 262609375) {
			continue;
		} else {
#if 0
			printf("%lld\n", score_commit);
			printf("%lld\n", score_compensation);
			printf("%lld\n", score_compensation2);
#endif
			SUBTEST_PRINT_RESULT(2, TEST_FAILURE);
			return 1;
		}
	}
	SUBTEST_PRINT_RESULT(2, TEST_SUCCESS);
	return 0;
}

int
main(int argc, char *argv[])
{
	int test1_res;
	int test2_res;

	txc_global_init();
	txc_thread_init();

	test1_res = test1();
	test2_res = test2();

	if (test1_res || test2_res) {
		TEST_PRINT_RESULT(TEST_FAILURE);
		return 1;
	} else {
		TEST_PRINT_RESULT(TEST_SUCCESS);
		return 0; 
	}
}
