/*
 * test_sc_hysteresis.c -- requirements-based tests for sc_hysteresis.
 *
 * Each test names the requirement(s) it verifies. The update-cycle tests
 * are chosen to give MC/DC over the two compound decisions in
 * sc_hysteresis_update (see requirements/sc_hysteresis.md).
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_hysteresis.h"

/* Shared fixture: assert at >= 100, clear at <= 50. */
static const sc_hysteresis_config_t cfg_100_50 = { 50, 100 };

/* HLR-HYS-1 / LLR-HYS-1,10 -- init produces the cleared output. */
static void hys_init_is_cleared(void)
{
    sc_hysteresis_state_t st;
    SC_CHECK_EQ(sc_hysteresis_init(&st), SC_OK);
    SC_CHECK(sc_hysteresis_output(&st) == false);
}

/* HLR-HYS-2 / LLR-HYS-2 -- rising to the high threshold asserts. Decision 1
 * with both conditions true. */
static void hys_asserts_at_high_threshold(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 100) == true);   /* == boundary */
}

/* HLR-HYS-4 / LLR-HYS-4 -- from cleared, an input below the high threshold
 * holds cleared. Decision 1: first condition true, second false. */
static void hys_holds_cleared_below_high(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 99) == false);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 75) == false);
}

/* HLR-HYS-4 / LLR-HYS-4 -- once asserted, an input between the thresholds
 * holds asserted. Decision 1 first condition false; decision 2 second
 * condition false. */
static void hys_holds_asserted_between(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    (void)sc_hysteresis_update(&cfg_100_50, &st, 120);               /* assert */
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 75) == true);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 51) == true);
}

/* HLR-HYS-3 / LLR-HYS-3 -- falling to the low threshold clears. Decision 2
 * with both conditions true. */
static void hys_clears_at_low_threshold(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    (void)sc_hysteresis_update(&cfg_100_50, &st, 200);               /* assert */
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 50) == false);   /* == boundary */
}

/* HLR-HYS-4 -- while asserted, an input still above the high threshold
 * holds asserted (decision 1 first condition false, decision 2 reached with
 * second condition false). */
static void hys_holds_asserted_above_high(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    (void)sc_hysteresis_update(&cfg_100_50, &st, 150);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 300) == true);
}

/* HLR-HYS-5 -- a full assert/clear/re-assert cycle behaves monotonically. */
static void hys_full_cycle(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 40)  == false);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 100) == true);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 80)  == true);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 50)  == false);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 60)  == false);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, &st, 100) == true);
}

/* HLR-HYS-6 -- equal thresholds give a plain comparator with no dead band. */
static void hys_zero_width_band(void)
{
    static const sc_hysteresis_config_t cfg_eq = { 0, 0 };
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    SC_CHECK(sc_hysteresis_update(&cfg_eq, &st, 0)  == true);
    SC_CHECK(sc_hysteresis_update(&cfg_eq, &st, -1) == false);
    SC_CHECK(sc_hysteresis_update(&cfg_eq, &st, 1)  == true);
}

/* LLR-HYS-5 -- update is null-safe and returns false. */
static void hys_update_null_safe(void)
{
    sc_hysteresis_state_t st;
    SC_MUST(sc_hysteresis_init(&st));
    SC_CHECK(sc_hysteresis_update(NULL, &st, 100) == false);
    SC_CHECK(sc_hysteresis_update(&cfg_100_50, NULL, 100) == false);
}

/* HLR-HYS-7 / LLR-HYS-7,8,9 -- config validation. */
static void hys_config_validation(void)
{
    static const sc_hysteresis_config_t good = { 10, 20 };
    static const sc_hysteresis_config_t inverted = { 20, 10 };
    SC_CHECK_EQ(sc_hysteresis_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_hysteresis_config_valid(&inverted), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_hysteresis_config_valid(&good), SC_OK);
}

/* LLR-HYS-10,11,12 -- init and output null handling. */
static void hys_init_and_output_null(void)
{
    sc_hysteresis_state_t st;
    SC_CHECK_EQ(sc_hysteresis_init(NULL), SC_ERR_NULL);
    SC_CHECK(sc_hysteresis_output(NULL) == false);
    SC_MUST(sc_hysteresis_init(&st));
    (void)sc_hysteresis_update(&cfg_100_50, &st, 500);
    SC_CHECK(sc_hysteresis_output(&st) == true);
}

const sc_test_case sc_suite_hysteresis[] = {
    { "hys_init_is_cleared",          hys_init_is_cleared          },
    { "hys_asserts_at_high_threshold",hys_asserts_at_high_threshold},
    { "hys_holds_cleared_below_high",  hys_holds_cleared_below_high  },
    { "hys_holds_asserted_between",    hys_holds_asserted_between    },
    { "hys_clears_at_low_threshold",   hys_clears_at_low_threshold   },
    { "hys_holds_asserted_above_high", hys_holds_asserted_above_high },
    { "hys_full_cycle",               hys_full_cycle               },
    { "hys_zero_width_band",           hys_zero_width_band           },
    { "hys_update_null_safe",          hys_update_null_safe          },
    { "hys_config_validation",         hys_config_validation         },
    { "hys_init_and_output_null",      hys_init_and_output_null      }
};
const unsigned sc_suite_hysteresis_count =
    (unsigned)(sizeof(sc_suite_hysteresis) / sizeof(sc_suite_hysteresis[0]));
