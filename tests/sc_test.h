/*
 * sc_test.h -- minimal dependency-free test harness for dal-c.
 *
 * C99, uses only <stdio.h> for the report. Each test file defines static
 * void(void) functions and exposes them as an sc_test_case[] table (see
 * sc_suites.h). Tests use SC_CHECK; the process exits non-zero on any
 * failure.
 */
#ifndef DAL_C_SC_TEST_H
#define DAL_C_SC_TEST_H

#include <stdio.h>

typedef void (*sc_test_fn)(void);

typedef struct
{
    const char *name;
    sc_test_fn  fn;
} sc_test_case;

/* per-run counters, defined once in tests/main.c */
extern unsigned    sc_test_checks;
extern unsigned    sc_test_failures;
extern const char *sc_test_current;

#define SC_CHECK(cond)                                                        \
    do {                                                                     \
        sc_test_checks++;                                                    \
        if (!(cond)) {                                                       \
            sc_test_failures++;                                              \
            (void)printf("  FAIL [%s] %s:%d  %s\n",                          \
                         sc_test_current, __FILE__, __LINE__, #cond);        \
        }                                                                   \
    } while (0)

#define SC_CHECK_EQ(a, b) SC_CHECK((a) == (b))

/* Setup call that must succeed: asserts the expression equals SC_OK. Also
 * consumes warn_unused_result returns cleanly. */
#define SC_MUST(expr) SC_CHECK((expr) == SC_OK)

/* Run one suite; returns the number of failures it added. */
unsigned sc_test_run(const sc_test_case *cases, unsigned count);

#endif /* DAL_C_SC_TEST_H */
