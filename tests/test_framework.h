#ifndef DC_TEST_FRAMEWORK_H
#define DC_TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    g_tests_run++; \
    printf("  %-50s ", #name); \
    test_##name(); \
    g_tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    %s:%d: assertion failed: %s\n", __FILE__, __LINE__, #cond); \
        g_tests_failed++; \
        g_tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL\n    %s:%d: expected %ld, got %ld\n", __FILE__, __LINE__, (long)(b), (long)(a)); \
        g_tests_failed++; \
        g_tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    const char *_a = (a); const char *_b = (b); \
    if (strcmp(_a, _b) != 0) { \
        printf("FAIL\n    %s:%d:\n    expected: \"%s\"\n    got:      \"%s\"\n", __FILE__, __LINE__, _b, _a); \
        g_tests_failed++; \
        g_tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_STRN_EQ(a, alen, b, blen) do { \
    if ((alen) != (blen) || memcmp((a), (b), (alen)) != 0) { \
        printf("FAIL\n    %s:%d: output mismatch (len %zu vs %zu)\n", __FILE__, __LINE__, (size_t)(alen), (size_t)(blen)); \
        g_tests_failed++; \
        g_tests_passed--; \
        return; \
    } \
} while(0)

#define TEST_SUITE(name) printf("\n%s:\n", name)

#define TEST_REPORT() do { \
    printf("\n---\nResults: %d passed, %d failed, %d total\n", g_tests_passed, g_tests_failed, g_tests_run); \
    return g_tests_failed > 0 ? 1 : 0; \
} while(0)

#endif /* DC_TEST_FRAMEWORK_H */
