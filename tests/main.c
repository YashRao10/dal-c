/*
 * main.c -- dal-c test runner. Executes every registered suite and prints a
 * one-line summary. Exit code 0 iff all checks passed.
 */
#include "sc_test.h"
#include "sc_suites.h"

unsigned    sc_test_checks   = 0u;
unsigned    sc_test_failures = 0u;
const char *sc_test_current  = "";

unsigned sc_test_run(const sc_test_case *cases, unsigned count)
{
    unsigned before = sc_test_failures;
    unsigned i;

    for (i = 0u; i < count; i++)
    {
        sc_test_current = cases[i].name;
        cases[i].fn();
    }

    return sc_test_failures - before;
}

int main(void)
{
    (void)sc_test_run(sc_suite_sat,        sc_suite_sat_count);
    (void)sc_test_run(sc_suite_hysteresis, sc_suite_hysteresis_count);
    (void)sc_test_run(sc_suite_ringbuf,    sc_suite_ringbuf_count);
    (void)sc_test_run(sc_suite_ratelimit,  sc_suite_ratelimit_count);

    (void)printf("\n%u checks, %u failure(s)\n",
                 sc_test_checks, sc_test_failures);

    return (sc_test_failures == 0u) ? 0 : 1;
}
