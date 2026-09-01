/*
 * test_sc_ratelimit.c -- requirements-based tests for sc_ratelimit.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_ratelimit.h"

/* rise <= 10/update, fall <= 20/update, output clamped to [0, 1000]. */
static const sc_ratelimit_config_t cfg_10_20 = { 10, 20, 0, 1000 };

/* HLR-RL-1 / LLR-RL-2 -- a large upward command is limited to max_step_up. */
static void rl_limits_rise(void)
{
    sc_ratelimit_state_t st;
    SC_MUST(sc_ratelimit_init(&st, 0));
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 500), 10);
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 500), 20);
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 500), 30);
}

/* HLR-RL-1 / LLR-RL-3 -- a large downward command is limited to max_step_down. */
static void rl_limits_fall(void)
{
    sc_ratelimit_state_t st;
    SC_MUST(sc_ratelimit_init(&st, 100));
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 0), 80);
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 0), 60);
}

/* HLR-RL-2 / LLR-RL-4 -- a change already within the limits passes straight
 * through to the target. */
static void rl_small_change_passes(void)
{
    sc_ratelimit_state_t st;
    SC_MUST(sc_ratelimit_init(&st, 100));
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 105), 105);
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 95), 95);
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 95), 95);   /* delta 0 */
}

/* HLR-RL-3 -- the output converges exactly onto a reachable target. */
static void rl_converges(void)
{
    sc_ratelimit_state_t st;
    int i;
    SC_MUST(sc_ratelimit_init(&st, 0));
    for (i = 0; i < 100; i++)
    {
        (void)sc_ratelimit_update(&cfg_10_20, &st, 55);
    }
    SC_CHECK_EQ(sc_ratelimit_output(&st), 55);
}

/* HLR-RL-4 / LLR-RL-7,8 -- the result is clamped to [out_min, out_max]. */
static void rl_output_clamp(void)
{
    sc_ratelimit_state_t st;
    int i;

    SC_MUST(sc_ratelimit_init(&st, 995));
    for (i = 0; i < 5; i++)
    {
        (void)sc_ratelimit_update(&cfg_10_20, &st, 100000);
    }
    SC_CHECK_EQ(sc_ratelimit_output(&st), 1000);   /* pinned at out_max */

    SC_MUST(sc_ratelimit_init(&st, 5));
    for (i = 0; i < 5; i++)
    {
        (void)sc_ratelimit_update(&cfg_10_20, &st, -100000);
    }
    SC_CHECK_EQ(sc_ratelimit_output(&st), 0);       /* pinned at out_min */
}

/* HLR-RL-4 -- an out-of-range initial value is pulled into range on the
 * first update even when the target is also out of range. */
static void rl_init_out_of_range(void)
{
    sc_ratelimit_state_t st;
    SC_MUST(sc_ratelimit_init(&st, 5000));            /* above out_max */
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, &st, 6000), 1000);
}

/* HLR-RL-5 -- extreme targets never wrap the output. With a full-width
 * slew limit the output advances monotonically toward the target and
 * converges within a few cycles, never overshooting or wrapping. */
static void rl_no_wrap_on_extremes(void)
{
    static const sc_ratelimit_config_t wide =
        { INT32_MAX, INT32_MAX, INT32_MIN, INT32_MAX };
    sc_ratelimit_state_t st;
    int32_t prev;
    int i;

    SC_MUST(sc_ratelimit_init(&st, INT32_MIN));
    prev = INT32_MIN;
    for (i = 0; i < 4; i++)
    {
        int32_t now = sc_ratelimit_update(&wide, &st, INT32_MAX);
        SC_CHECK(now >= prev);
        SC_CHECK(now <= INT32_MAX);
        prev = now;
    }
    SC_CHECK_EQ(sc_ratelimit_output(&st), INT32_MAX);

    SC_MUST(sc_ratelimit_init(&st, INT32_MAX));
    prev = INT32_MAX;
    for (i = 0; i < 4; i++)
    {
        int32_t now = sc_ratelimit_update(&wide, &st, INT32_MIN);
        SC_CHECK(now <= prev);
        SC_CHECK(now >= INT32_MIN);
        prev = now;
    }
    SC_CHECK_EQ(sc_ratelimit_output(&st), INT32_MIN);

    /* A single step that ends exactly on a rail does not overshoot it. */
    SC_MUST(sc_ratelimit_init(&st, INT32_MAX - 5));
    SC_CHECK_EQ(sc_ratelimit_update(&wide, &st, INT32_MAX), INT32_MAX);
    SC_MUST(sc_ratelimit_init(&st, INT32_MIN + 5));
    SC_CHECK_EQ(sc_ratelimit_update(&wide, &st, INT32_MIN), INT32_MIN);
}

/* HLR-RL-6 / LLR-RL-6 -- null cfg returns the current output; null state
 * returns 0. */
static void rl_null_safe(void)
{
    sc_ratelimit_state_t st;
    SC_MUST(sc_ratelimit_init(&st, 42));
    SC_CHECK_EQ(sc_ratelimit_update(NULL, &st, 999), 42);
    SC_CHECK_EQ(sc_ratelimit_update(&cfg_10_20, NULL, 999), 0);
    SC_CHECK_EQ(sc_ratelimit_output(NULL), 0);
}

/* HLR-RL-7 / LLR-RL-11,12,13,14 -- config validation. */
static void rl_config_validation(void)
{
    static const sc_ratelimit_config_t neg_up   = { -1, 10, 0, 100 };
    static const sc_ratelimit_config_t neg_down  = { 10, -1, 0, 100 };
    static const sc_ratelimit_config_t bad_clamp = { 10, 10, 100, 0 };
    static const sc_ratelimit_config_t good      = { 10, 10, 0, 100 };

    SC_CHECK_EQ(sc_ratelimit_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ratelimit_config_valid(&neg_up), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_ratelimit_config_valid(&neg_down), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_ratelimit_config_valid(&bad_clamp), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_ratelimit_config_valid(&good), SC_OK);
}

/* LLR-RL-15,16 -- init null handling and value set. */
static void rl_init(void)
{
    sc_ratelimit_state_t st;
    SC_CHECK_EQ(sc_ratelimit_init(NULL, 0), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ratelimit_init(&st, -7), SC_OK);
    SC_CHECK_EQ(sc_ratelimit_output(&st), -7);
}

const sc_test_case sc_suite_ratelimit[] = {
    { "rl_limits_rise",         rl_limits_rise         },
    { "rl_limits_fall",         rl_limits_fall         },
    { "rl_small_change_passes", rl_small_change_passes },
    { "rl_converges",           rl_converges           },
    { "rl_output_clamp",        rl_output_clamp        },
    { "rl_init_out_of_range",   rl_init_out_of_range   },
    { "rl_no_wrap_on_extremes", rl_no_wrap_on_extremes },
    { "rl_null_safe",           rl_null_safe           },
    { "rl_config_validation",   rl_config_validation   },
    { "rl_init",                rl_init                }
};
const unsigned sc_suite_ratelimit_count =
    (unsigned)(sizeof(sc_suite_ratelimit) / sizeof(sc_suite_ratelimit[0]));
