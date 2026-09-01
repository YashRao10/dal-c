/*
 * sc_suites.h -- registry of test suites.
 *
 * Each test file defines its table and count; main.c walks the list.
 * Keeping the declarations in one header gives one place to see every suite
 * and keeps -Wmissing-prototypes-style checks satisfied.
 */
#ifndef DAL_C_SC_SUITES_H
#define DAL_C_SC_SUITES_H

#include "sc_test.h"

extern const sc_test_case sc_suite_sat[];
extern const unsigned     sc_suite_sat_count;

extern const sc_test_case sc_suite_hysteresis[];
extern const unsigned     sc_suite_hysteresis_count;

extern const sc_test_case sc_suite_ringbuf[];
extern const unsigned     sc_suite_ringbuf_count;

extern const sc_test_case sc_suite_ratelimit[];
extern const unsigned     sc_suite_ratelimit_count;

#endif /* DAL_C_SC_SUITES_H */
