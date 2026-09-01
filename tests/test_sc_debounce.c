/*
 * test_sc_debounce.c -- requirements-based tests for sc_debounce.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_debounce.h"

static const sc_debounce_config_t CFG3 = { 3u };   /* need 3 to accept a change */

/* HLR-DB-1 / LLR-DB-6,7,8 -- config validation. */
static void db_config_validation(void)
{
    static const sc_debounce_config_t zero = { 0u };
    SC_CHECK_EQ(sc_debounce_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_debounce_config_valid(&zero), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_debounce_config_valid(&CFG3), SC_OK);
}

/* HLR-DB-2 / LLR-DB-1,9 -- init and null-safety. */
static void db_init_and_null(void)
{
    sc_debounce_state_t st;
    SC_CHECK_EQ(sc_debounce_init(NULL, false), SC_ERR_NULL);
    SC_CHECK_EQ(sc_debounce_init(&st, true), SC_OK);
    SC_CHECK(st.output == 1u);
    SC_CHECK_EQ(st.counter, 0u);
    SC_CHECK(sc_debounce_update(NULL, &st, false) == false);
    SC_CHECK(sc_debounce_update(&CFG3, NULL, false) == false);
}

/* HLR-DB-3 -- a change is only accepted after `threshold` consecutive
 * disagreeing samples. */
static void db_accepts_after_threshold(void)
{
    sc_debounce_state_t st;
    SC_MUST(sc_debounce_init(&st, false));
    SC_CHECK(sc_debounce_update(&CFG3, &st, true) == false);   /* run 1 */
    SC_CHECK(sc_debounce_update(&CFG3, &st, true) == false);   /* run 2 */
    SC_CHECK(sc_debounce_update(&CFG3, &st, true) == true);    /* run 3 -> accept */
    SC_CHECK(sc_debounce_update(&CFG3, &st, true) == true);    /* holds */
}

/* HLR-DB-4 -- a burst shorter than the threshold is rejected and the run
 * counter resets on the first agreeing sample. */
static void db_rejects_short_burst(void)
{
    sc_debounce_state_t st;
    SC_MUST(sc_debounce_init(&st, false));
    SC_CHECK(sc_debounce_update(&CFG3, &st, true)  == false);  /* run 1 */
    SC_CHECK(sc_debounce_update(&CFG3, &st, true)  == false);  /* run 2 */
    SC_CHECK(sc_debounce_update(&CFG3, &st, false) == false);  /* agrees: reset */
    SC_CHECK(sc_debounce_update(&CFG3, &st, true)  == false);  /* run 1 again */
    SC_CHECK(sc_debounce_update(&CFG3, &st, true)  == false);
    SC_CHECK(sc_debounce_update(&CFG3, &st, true)  == true);   /* now accepts */
}

/* HLR-DB-3 -- threshold of 1 accepts immediately. */
static void db_threshold_one(void)
{
    static const sc_debounce_config_t one = { 1u };
    sc_debounce_state_t st;
    SC_MUST(sc_debounce_init(&st, false));
    SC_CHECK(sc_debounce_update(&one, &st, true) == true);
    SC_CHECK(sc_debounce_update(&one, &st, false) == false);
}

/* HLR-DB-4 -- an already-agreeing sample keeps the counter at zero. */
static void db_steady_state(void)
{
    sc_debounce_state_t st;
    SC_MUST(sc_debounce_init(&st, true));
    SC_CHECK(sc_debounce_update(&CFG3, &st, true) == true);
    SC_CHECK_EQ(st.counter, 0u);
    SC_CHECK(sc_debounce_update(&CFG3, &st, true) == true);
    SC_CHECK_EQ(st.counter, 0u);
}

const sc_test_case sc_suite_debounce[] = {
    { "db_config_validation",       db_config_validation       },
    { "db_init_and_null",           db_init_and_null           },
    { "db_accepts_after_threshold", db_accepts_after_threshold },
    { "db_rejects_short_burst",     db_rejects_short_burst     },
    { "db_threshold_one",           db_threshold_one           },
    { "db_steady_state",            db_steady_state            }
};
const unsigned sc_suite_debounce_count =
    (unsigned)(sizeof(sc_suite_debounce) / sizeof(sc_suite_debounce[0]));
