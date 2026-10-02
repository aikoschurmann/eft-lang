#include "framework.h"

int g_tests_run = 0;
int g_tests_passed = 0;

extern void suite_datastructures(void);
extern void suite_lexer(void);

int main(void) {
    printf("========================================\n");
    printf(" RUNNING COMPILER TEST SUITE\n");
    printf("========================================\n");

    suite_datastructures();
    suite_lexer();

    printf("========================================\n");
    printf("\033[32m SUCCESS\033[0m: %d/%d tests passed.\n", g_tests_passed, g_tests_run);
    printf("========================================\n");
    return 0;
}
