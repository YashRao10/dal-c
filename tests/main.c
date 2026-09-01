/*
 * main.c -- dal-c test runner. Executes every registered suite, prints a
 * per-suite and total summary, and (with --report) a machine-readable block
 * that tools/gen_report.py consumes. Exit code 0 iff all checks passed.
 */
#include <string.h>

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

typedef struct
{
    const char        *name;
    const sc_test_case *cases;
    unsigned           count;
} suite_entry;

#define SUITE_COUNT 12u

int main(int argc, char **argv)
{
    suite_entry suites[SUITE_COUNT];
    unsigned s;
    int report = (argc > 1) && (strcmp(argv[1], "--report") == 0);

    suites[0].name = "sc_sat";
    suites[0].cases = sc_suite_sat;
    suites[0].count = sc_suite_sat_count;
    suites[1].name = "sc_fixed";
    suites[1].cases = sc_suite_fixed;
    suites[1].count = sc_suite_fixed_count;
    suites[2].name = "sc_crc";
    suites[2].cases = sc_suite_crc;
    suites[2].count = sc_suite_crc_count;
    suites[3].name = "sc_hysteresis";
    suites[3].cases = sc_suite_hysteresis;
    suites[3].count = sc_suite_hysteresis_count;
    suites[4].name = "sc_ringbuf";
    suites[4].cases = sc_suite_ringbuf;
    suites[4].count = sc_suite_ringbuf_count;
    suites[5].name = "sc_ratelimit";
    suites[5].cases = sc_suite_ratelimit;
    suites[5].count = sc_suite_ratelimit_count;
    suites[6].name = "sc_lut";
    suites[6].cases = sc_suite_lut;
    suites[6].count = sc_suite_lut_count;
    suites[7].name = "sc_debounce";
    suites[7].cases = sc_suite_debounce;
    suites[7].count = sc_suite_debounce_count;
    suites[8].name = "sc_sm";
    suites[8].cases = sc_suite_sm;
    suites[8].count = sc_suite_sm_count;
    suites[9].name = "sc_sched";
    suites[9].cases = sc_suite_sched;
    suites[9].count = sc_suite_sched_count;
    suites[10].name = "sc_cobs";
    suites[10].cases = sc_suite_frame;
    suites[10].count = sc_suite_frame_count;
    suites[11].name = "sc_pid";
    suites[11].cases = sc_suite_pid;
    suites[11].count = sc_suite_pid_count;

    for (s = 0u; s < SUITE_COUNT; s++)
    {
        unsigned before_checks = sc_test_checks;
        unsigned fails = sc_test_run(suites[s].cases, suites[s].count);
        (void)printf("  %-14s %3u tests  %4u checks  %s\n",
                     suites[s].name, suites[s].count,
                     sc_test_checks - before_checks,
                     (fails == 0u) ? "PASS" : "FAIL");
    }

    (void)printf("\n%u checks, %u failure(s)\n",
                 sc_test_checks, sc_test_failures);

    if (report)
    {
        (void)printf("\n#REPORT\n");
        for (s = 0u; s < SUITE_COUNT; s++)
        {
            (void)printf("suite\t%s\t%u\n", suites[s].name, suites[s].count);
        }
        (void)printf("total\t%u\t%u\n", sc_test_checks, sc_test_failures);
        (void)printf("#END\n");
    }

    return (sc_test_failures == 0u) ? 0 : 1;
}
