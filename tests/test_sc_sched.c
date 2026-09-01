/*
 * test_sc_sched.c -- requirements-based tests for sc_sched.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_sched.h"

typedef struct { int a; int b; int c; int order; int a_at; } sched_ctx;

static void task_a(void *p) { sched_ctx *x = p; x->a++; x->a_at = x->order; x->order++; }
static void task_b(void *p) { sched_ctx *x = p; x->b++; x->order++; }
static void task_c(void *p) { sched_ctx *x = p; x->c++; x->order++; }

/* A every tick; B on even ticks; C every 3rd tick starting at tick 1. */
static const sc_sched_slot SLOTS[] = {
    { task_a, 1u, 0u },
    { task_b, 2u, 0u },
    { task_c, 3u, 1u }
};
static const sc_sched_config_t CFG = { SLOTS, 3u };

/* HLR-SCH-1 / LLR-SCH-6,7,8 -- config validation. */
static void sched_config_validation(void)
{
    static const sc_sched_slot bad_run[1]    = { { NULL, 1u, 0u } };
    static const sc_sched_slot bad_period[1] = { { task_a, 0u, 0u } };
    static const sc_sched_slot bad_phase[1]  = { { task_a, 2u, 2u } };
    static const sc_sched_config_t no_slots = { NULL, 1u };
    static const sc_sched_config_t empty    = { SLOTS, 0u };
    sc_sched_config_t c;

    SC_CHECK_EQ(sc_sched_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sched_config_valid(&no_slots), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sched_config_valid(&empty), SC_ERR_PARAM);
    c.slots = bad_run;    c.n = 1u; SC_CHECK_EQ(sc_sched_config_valid(&c), SC_ERR_NULL);
    c.slots = bad_period; c.n = 1u; SC_CHECK_EQ(sc_sched_config_valid(&c), SC_ERR_PARAM);
    c.slots = bad_phase;  c.n = 1u; SC_CHECK_EQ(sc_sched_config_valid(&c), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_sched_config_valid(&CFG), SC_OK);
}

/* HLR-SCH-2 / LLR-SCH-1,5 -- init and null-safety. */
static void sched_init_and_null(void)
{
    sc_sched_state_t st;
    sched_ctx ctx = { 0, 0, 0, 0, -1 };
    SC_CHECK_EQ(sc_sched_init(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sched_init(&st), SC_OK);
    SC_CHECK_EQ(st.tick, 0u);
    SC_CHECK_EQ(sc_sched_tick(NULL, &st, &ctx), 0u);
    SC_CHECK_EQ(sc_sched_tick(&CFG, NULL, &ctx), 0u);
    SC_CHECK_EQ(st.tick, 0u);   /* not advanced */
}

/* HLR-SCH-3 -- each task runs exactly on its due ticks over a full period
 * (lcm(1,2,3) = 6, run 12 ticks). */
static void sched_periods(void)
{
    sc_sched_state_t st;
    sched_ctx ctx = { 0, 0, 0, 0, -1 };
    int i;
    SC_MUST(sc_sched_init(&st));
    for (i = 0; i < 12; i++)
    {
        (void)sc_sched_tick(&CFG, &st, &ctx);
    }
    SC_CHECK_EQ(ctx.a, 12);   /* every tick        */
    SC_CHECK_EQ(ctx.b, 6);    /* ticks 0,2,4,6,8,10 */
    SC_CHECK_EQ(ctx.c, 4);    /* ticks 1,4,7,10     */
    SC_CHECK_EQ(st.tick, 12u);
}

/* HLR-SCH-3 -- return value is the number of tasks run that tick, and the
 * order within a tick is table order. */
static void sched_return_and_order(void)
{
    sc_sched_state_t st;
    sched_ctx ctx = { 0, 0, 0, 0, -1 };
    SC_MUST(sc_sched_init(&st));

    SC_CHECK_EQ(sc_sched_tick(&CFG, &st, &ctx), 2u);   /* tick 0: A, B  */
    SC_CHECK_EQ(ctx.a_at, 0);                          /* A ran first   */

    ctx.order = 0;
    SC_CHECK_EQ(sc_sched_tick(&CFG, &st, &ctx), 2u);   /* tick 1: A, C  */

    ctx.order = 0;
    SC_CHECK_EQ(sc_sched_tick(&CFG, &st, &ctx), 2u);   /* tick 2: A, B  */

    ctx.order = 0;
    SC_CHECK_EQ(sc_sched_tick(&CFG, &st, &ctx), 1u);   /* tick 3: A only */
}

const sc_test_case sc_suite_sched[] = {
    { "sched_config_validation", sched_config_validation },
    { "sched_init_and_null",     sched_init_and_null     },
    { "sched_periods",           sched_periods           },
    { "sched_return_and_order",  sched_return_and_order  }
};
const unsigned sc_suite_sched_count =
    (unsigned)(sizeof(sc_suite_sched) / sizeof(sc_suite_sched[0]));
