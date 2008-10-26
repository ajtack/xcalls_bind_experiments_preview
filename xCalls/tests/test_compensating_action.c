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

char *test_name = "test_compensating_action";
int seq_num = 0;	
int retries;
int score; 
int godel[] = {2,3,5,7,11,13,17,23,31}; 

void 
test1_compensating_action_1(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 0); 
	score *= pow(godel[0], ++seq_num);
}

void 
test1_compensating_action_2(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 0); 
	score *= pow(godel[1], ++seq_num);
}

int 
test1() 
{
	retries = 0;
	seq_num = 0;
	score = 1;
	
	XACT_BEGIN(xact_commit_action1)
		__tm_waiver { seq_num++;	}
		XACT_BEGIN(xact_commit_action2)
			txc_register_compensating_action(test1_compensating_action_1, 0, NULL);
			__tm_waiver {
				if (retries++ == 0) {
					XACT_RETRY
				}
			}
		XACT_END(xact_commit_action2)
		txc_register_compensating_action(test1_compensating_action_2, 0, NULL);
	XACT_END(xact_commit_action1)
	if (score == 4) {
		SUBTEST_PRINT_RESULT(1, TEST_SUCCESS);
		return 0;
	} else {
		SUBTEST_PRINT_RESULT(1, TEST_FAILURE);
		return 1;
	}
}


int 
test2() 
{
	retries = 0;
	seq_num = 0;
	score = 1;
	
	XACT_BEGIN(xact_commit_action1)
		__tm_waiver { seq_num++;	}
		XACT_BEGIN(xact_commit_action2)
			txc_register_compensating_action(test1_compensating_action_1, 0, NULL);
		XACT_END(xact_commit_action2)
		txc_register_compensating_action(test1_compensating_action_2, 0, NULL);
		__tm_waiver {
			if (retries++ == 0) {
				XACT_RETRY
			}
		}
	XACT_END(xact_commit_action1)
	if (score == 72) {
		SUBTEST_PRINT_RESULT(2, TEST_SUCCESS);
		return 0;
	} else {
		SUBTEST_PRINT_RESULT(2, TEST_FAILURE);
		return 1;
	}
}

void 
test3_compensating_action_1(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 1); 
	int my_seq_num = (int) params[0];
	score *= pow(godel[0], my_seq_num);
}

void 
test3_compensating_action_2(unsigned int numParams, unsigned int *params)
{
	assert(numParams == 1); 
	int my_seq_num = (int) params[0];
	score *= pow(godel[1], my_seq_num);
}

int 
test3() 
{
	retries = 0;
	seq_num = 0;
	score = 1;
	unsigned int inputarray[6];
	
	XACT_BEGIN(xact_commit_action1)
		__tm_waiver { seq_num++;	}
		XACT_BEGIN(xact_commit_action2)
			__tm_waiver { seq_num++; }
			inputarray[0] = (unsigned int) seq_num;
			txc_register_compensating_action(test3_compensating_action_1, 1, inputarray);
		XACT_END(xact_commit_action2)
		__tm_waiver { seq_num++; }
		inputarray[0] = (unsigned int) seq_num;
		txc_register_compensating_action(test3_compensating_action_2, 1, inputarray);
		__tm_waiver {
			if (retries++ == 0) {
				XACT_RETRY
			}
		}
	XACT_END(xact_commit_action1)
	if (score == 108) {
		SUBTEST_PRINT_RESULT(3, TEST_SUCCESS);
		return 0;
	} else {
		printf("score = %d\n", score); 
		SUBTEST_PRINT_RESULT(3, TEST_FAILURE);
		return 1;
	}
}

int
main(int argc, char *argv[])
{
	int test1_res;
	int test2_res;
	int test3_res;

	txc_global_init();
	txc_thread_init();

	test1_res = test1();
	test2_res = test2();
	test3_res = test3();

	if (test1_res || test2_res || test3_res ) {
		TEST_PRINT_RESULT(TEST_FAILURE);
		return 1;
	} else {
		TEST_PRINT_RESULT(TEST_SUCCESS);
		return 0; 
	}
}
