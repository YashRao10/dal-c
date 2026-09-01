/*
 * sc_suites.h -- registry of test suites.
 *
 * Each test file defines its table and count; main.c walks the list.
 */
#ifndef DAL_C_SC_SUITES_H
#define DAL_C_SC_SUITES_H

#include "sc_test.h"

extern const sc_test_case sc_suite_sat[];
extern const unsigned     sc_suite_sat_count;

extern const sc_test_case sc_suite_fixed[];
extern const unsigned     sc_suite_fixed_count;

extern const sc_test_case sc_suite_crc[];
extern const unsigned     sc_suite_crc_count;

extern const sc_test_case sc_suite_hysteresis[];
extern const unsigned     sc_suite_hysteresis_count;

extern const sc_test_case sc_suite_ringbuf[];
extern const unsigned     sc_suite_ringbuf_count;

extern const sc_test_case sc_suite_ratelimit[];
extern const unsigned     sc_suite_ratelimit_count;

extern const sc_test_case sc_suite_lut[];
extern const unsigned     sc_suite_lut_count;

extern const sc_test_case sc_suite_debounce[];
extern const unsigned     sc_suite_debounce_count;

extern const sc_test_case sc_suite_median[];
extern const unsigned     sc_suite_median_count;

extern const sc_test_case sc_suite_sm[];
extern const unsigned     sc_suite_sm_count;

extern const sc_test_case sc_suite_sched[];
extern const unsigned     sc_suite_sched_count;

extern const sc_test_case sc_suite_frame[];
extern const unsigned     sc_suite_frame_count;

extern const sc_test_case sc_suite_pid[];
extern const unsigned     sc_suite_pid_count;

extern const sc_test_case sc_suite_vote[];
extern const unsigned     sc_suite_vote_count;

extern const sc_test_case sc_suite_watchdog[];
extern const unsigned     sc_suite_watchdog_count;

#endif /* DAL_C_SC_SUITES_H */
