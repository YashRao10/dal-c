/*
 * test_sc_sm.c -- requirements-based tests for sc_sm.
 *
 * A powered-door state machine: CLOSED -> OPENING -> OPEN -> CLOSING ->
 * CLOSED, with a mid-travel reverse. Actions bump counters in a context
 * struct so the tests can confirm they ran.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_sm.h"

enum { ST_CLOSED = 0, ST_OPENING = 1, ST_OPEN = 2, ST_CLOSING = 3 };
enum { EV_OPEN = 0, EV_CLOSE = 1, EV_DONE = 2 };

typedef struct { int fwd; int rev; int stop; } door_ctx;

static void act_fwd(void *c)  { ((door_ctx *)c)->fwd++;  }
static void act_rev(void *c)  { ((door_ctx *)c)->rev++;  }
static void act_stop(void *c) { ((door_ctx *)c)->stop++; }

static const sc_sm_transition DOOR[] = {
    { ST_CLOSED,  EV_OPEN,  ST_OPENING, act_fwd  },
    { ST_OPENING, EV_DONE,  ST_OPEN,    act_stop },
    { ST_OPENING, EV_CLOSE, ST_CLOSING, act_rev  },   /* reverse mid-travel */
    { ST_OPEN,    EV_CLOSE, ST_CLOSING, act_rev  },
    { ST_CLOSING, EV_DONE,  ST_CLOSED,  act_stop },
    { ST_OPEN,    EV_OPEN,  ST_OPEN,    NULL     }     /* self-loop, no action */
};
static const sc_sm_config_t DOOR_CFG = { DOOR, 6u, ST_CLOSED };

/* HLR-SM-1 / LLR-SM-7,8,9 -- config validation. */
static void sm_config_validation(void)
{
    static const sc_sm_config_t no_tab = { NULL, 3u, 0u };
    static const sc_sm_config_t empty  = { DOOR, 0u, 0u };
    SC_CHECK_EQ(sc_sm_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sm_config_valid(&no_tab), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sm_config_valid(&empty), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_sm_config_valid(&DOOR_CFG), SC_OK);
}

/* HLR-SM-2 / LLR-SM-1,6,10,11 -- init, state accessor, null-safety. */
static void sm_init_and_state(void)
{
    sc_sm_state_t sm;
    SC_CHECK_EQ(sc_sm_init(NULL, &DOOR_CFG), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sm_init(&sm, NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_sm_init(&sm, &DOOR_CFG), SC_OK);
    SC_CHECK_EQ(sc_sm_state(&sm), ST_CLOSED);
    SC_CHECK_EQ(sc_sm_state(NULL), 0xFFFFu);
}

/* HLR-SM-3 -- a matching event fires the transition, runs the action, and
 * moves the state. */
static void sm_transition_fires(void)
{
    sc_sm_state_t sm;
    door_ctx ctx = { 0, 0, 0 };
    SC_MUST(sc_sm_init(&sm, &DOOR_CFG));

    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_OPEN, &ctx) == true);
    SC_CHECK_EQ(sc_sm_state(&sm), ST_OPENING);
    SC_CHECK_EQ(ctx.fwd, 1);

    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_DONE, &ctx) == true);
    SC_CHECK_EQ(sc_sm_state(&sm), ST_OPEN);
    SC_CHECK_EQ(ctx.stop, 1);
}

/* HLR-SM-4 -- an event with no matching row from the current state is
 * ignored: no state change, no action, returns false. */
static void sm_unknown_event_ignored(void)
{
    sc_sm_state_t sm;
    door_ctx ctx = { 0, 0, 0 };
    SC_MUST(sc_sm_init(&sm, &DOOR_CFG));

    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_DONE, &ctx) == false);   /* no row for CLOSED+DONE */
    SC_CHECK_EQ(sc_sm_state(&sm), ST_CLOSED);
    SC_CHECK_EQ(ctx.fwd + ctx.rev + ctx.stop, 0);

    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_CLOSE, &ctx) == false);  /* CLOSED+CLOSE: none */
    SC_CHECK_EQ(sc_sm_state(&sm), ST_CLOSED);

    /* reach OPEN, then deliver an event whose rows match `from` but not
     * `event` -- the scan runs off the end and the state holds */
    (void)sc_sm_dispatch(&DOOR_CFG, &sm, EV_OPEN, &ctx);
    (void)sc_sm_dispatch(&DOOR_CFG, &sm, EV_DONE, &ctx);
    SC_CHECK_EQ(sc_sm_state(&sm), ST_OPEN);
    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_DONE, &ctx) == false);
    SC_CHECK_EQ(sc_sm_state(&sm), ST_OPEN);
}

/* HLR-SM-3 / LLR-SM-3 -- a transition whose action is NULL still fires. */
static void sm_null_action_transition(void)
{
    sc_sm_state_t sm;
    door_ctx ctx = { 0, 0, 0 };
    SC_MUST(sc_sm_init(&sm, &DOOR_CFG));
    (void)sc_sm_dispatch(&DOOR_CFG, &sm, EV_OPEN, &ctx);
    (void)sc_sm_dispatch(&DOOR_CFG, &sm, EV_DONE, &ctx);   /* now ST_OPEN */

    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_OPEN, &ctx) == true);    /* self-loop */
    SC_CHECK_EQ(sc_sm_state(&sm), ST_OPEN);
    SC_CHECK_EQ(ctx.fwd, 1);   /* unchanged -- the self-loop has no action */
}

/* HLR-SM-4 / LLR-SM-5 -- null arguments return false. */
static void sm_null_safe(void)
{
    sc_sm_state_t sm;
    SC_MUST(sc_sm_init(&sm, &DOOR_CFG));
    SC_CHECK(sc_sm_dispatch(NULL, &sm, EV_OPEN, NULL) == false);
    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, NULL, EV_OPEN, NULL) == false);
}

/* HLR-SM-3 -- full cycle, mid-travel reverse, back to CLOSED. */
static void sm_full_cycle(void)
{
    sc_sm_state_t sm;
    door_ctx ctx = { 0, 0, 0 };
    SC_MUST(sc_sm_init(&sm, &DOOR_CFG));

    (void)sc_sm_dispatch(&DOOR_CFG, &sm, EV_OPEN, &ctx);    /* -> OPENING */
    SC_CHECK(sc_sm_dispatch(&DOOR_CFG, &sm, EV_CLOSE, &ctx) == true);  /* reverse -> CLOSING */
    SC_CHECK_EQ(sc_sm_state(&sm), ST_CLOSING);
    SC_CHECK_EQ(ctx.rev, 1);
    (void)sc_sm_dispatch(&DOOR_CFG, &sm, EV_DONE, &ctx);    /* -> CLOSED */
    SC_CHECK_EQ(sc_sm_state(&sm), ST_CLOSED);
    SC_CHECK_EQ(ctx.stop, 1);
}

const sc_test_case sc_suite_sm[] = {
    { "sm_config_validation",     sm_config_validation     },
    { "sm_init_and_state",        sm_init_and_state        },
    { "sm_transition_fires",      sm_transition_fires      },
    { "sm_unknown_event_ignored", sm_unknown_event_ignored },
    { "sm_null_action_transition",sm_null_action_transition},
    { "sm_null_safe",             sm_null_safe             },
    { "sm_full_cycle",            sm_full_cycle            }
};
const unsigned sc_suite_sm_count =
    (unsigned)(sizeof(sc_suite_sm) / sizeof(sc_suite_sm[0]));
