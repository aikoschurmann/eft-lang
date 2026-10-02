#pragma once
#include <stdio.h>
#include <stdlib.h>

extern int g_tests_run;
extern int g_tests_passed;

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "\n\033[31mFAIL\033[0m: %s:%d: Assertion '%s' failed.\n", __FILE__, __LINE__, #cond); \
            abort(); \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        g_tests_run++; \
        printf("  %-35s", #test_func "..."); \
        fflush(stdout); \
        test_func(); \
        printf("\033[32mOK\033[0m\n"); \
        g_tests_passed++; \
    } while(0)
