#ifndef _TEST_UTIL_H
#define _TEST_UTIL_H

#define TEST_OUTPUT stderr
#define TEST_SUCCESS 0
#define TEST_FAILURE 1

char *test_result_str[] = {"SUCCESS", "FAILURE"};


#define SUBTEST_PRINT_RESULT(subtest, result)																\
	fprintf(TEST_OUTPUT, "%s: Subtest %d: %s\n", 															\
														test_name, subtest, test_result_str[result]);

#define TEST_PRINT_RESULT(result)																						\
	fprintf(TEST_OUTPUT, "%s: %s\n", 																					\
														test_name, test_result_str[result]);


#endif
