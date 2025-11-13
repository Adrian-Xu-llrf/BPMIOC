/**
 * @file test_framework.h
 * @brief Simple unit testing framework for BPM IOC
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <math.h>

/* ANSI color codes for output */
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_RESET   "\033[0m"

/* Test statistics */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
} TestStats;

static TestStats test_stats = {0, 0, 0, 0};
static char current_test_name[256] = "";

/* Macros for test definition */
#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        snprintf(current_test_name, sizeof(current_test_name), #name); \
        printf(COLOR_BLUE "[ RUN      ]" COLOR_RESET " %s\n", current_test_name); \
        test_##name(); \
    } \
    static void test_##name(void)

/* Assertion macros */
#define ASSERT_TRUE(condition) \
    do { \
        test_stats.total_tests++; \
        if (!(condition)) { \
            printf(COLOR_RED "[  FAILED  ]" COLOR_RESET " %s:%d: Assertion '%s' failed\n", \
                   __FILE__, __LINE__, #condition); \
            test_stats.failed_tests++; \
        } else { \
            test_stats.passed_tests++; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual) \
    do { \
        test_stats.total_tests++; \
        if ((expected) != (actual)) { \
            printf(COLOR_RED "[  FAILED  ]" COLOR_RESET " %s:%d: Expected %d but got %d\n", \
                   __FILE__, __LINE__, (int)(expected), (int)(actual)); \
            test_stats.failed_tests++; \
        } else { \
            test_stats.passed_tests++; \
        } \
    } while(0)

#define ASSERT_NE(not_expected, actual) \
    do { \
        test_stats.total_tests++; \
        if ((not_expected) == (actual)) { \
            printf(COLOR_RED "[  FAILED  ]" COLOR_RESET " %s:%d: Values should not be equal: %d\n", \
                   __FILE__, __LINE__, (int)(actual)); \
            test_stats.failed_tests++; \
        } else { \
            test_stats.passed_tests++; \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(expected, actual, epsilon) \
    do { \
        test_stats.total_tests++; \
        float diff = fabsf((expected) - (actual)); \
        if (diff > (epsilon)) { \
            printf(COLOR_RED "[  FAILED  ]" COLOR_RESET " %s:%d: Expected %.6f but got %.6f (diff=%.6f, epsilon=%.6f)\n", \
                   __FILE__, __LINE__, (float)(expected), (float)(actual), diff, (float)(epsilon)); \
            test_stats.failed_tests++; \
        } else { \
            test_stats.passed_tests++; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        test_stats.total_tests++; \
        if (strcmp((expected), (actual)) != 0) { \
            printf(COLOR_RED "[  FAILED  ]" COLOR_RESET " %s:%d: Expected '%s' but got '%s'\n", \
                   __FILE__, __LINE__, (expected), (actual)); \
            test_stats.failed_tests++; \
        } else { \
            test_stats.passed_tests++; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    ASSERT_TRUE((ptr) == NULL)

#define ASSERT_NOT_NULL(ptr) \
    ASSERT_TRUE((ptr) != NULL)

/* Test runner macros */
#define RUN_TEST(name) \
    do { \
        run_test_##name(); \
        printf(COLOR_GREEN "[       OK ]" COLOR_RESET " %s\n\n", #name); \
    } while(0)

#define TEST_SUITE_START(suite_name) \
    do { \
        printf(COLOR_YELLOW "===============================================\n"); \
        printf("Running test suite: %s\n", suite_name); \
        printf("===============================================" COLOR_RESET "\n\n"); \
        memset(&test_stats, 0, sizeof(test_stats)); \
    } while(0)

#define TEST_SUITE_END() \
    do { \
        printf(COLOR_YELLOW "===============================================\n"); \
        printf("Test Results:\n"); \
        printf("===============================================" COLOR_RESET "\n"); \
        printf("Total assertions: %d\n", test_stats.total_tests); \
        printf(COLOR_GREEN "Passed: %d\n" COLOR_RESET, test_stats.passed_tests); \
        if (test_stats.failed_tests > 0) { \
            printf(COLOR_RED "Failed: %d\n" COLOR_RESET, test_stats.failed_tests); \
        } else { \
            printf("Failed: 0\n"); \
        } \
        printf("Skipped: %d\n", test_stats.skipped_tests); \
        printf("\n"); \
        if (test_stats.failed_tests == 0) { \
            printf(COLOR_GREEN "All tests PASSED!\n" COLOR_RESET); \
        } else { \
            printf(COLOR_RED "Some tests FAILED!\n" COLOR_RESET); \
        } \
    } while(0)

#define SKIP_TEST(reason) \
    do { \
        printf(COLOR_YELLOW "[ SKIPPED  ]" COLOR_RESET " %s: %s\n", current_test_name, reason); \
        test_stats.skipped_tests++; \
        return; \
    } while(0)

/* Return value for main function */
#define TEST_RETURN_CODE() (test_stats.failed_tests == 0 ? 0 : 1)

#endif /* TEST_FRAMEWORK_H */
