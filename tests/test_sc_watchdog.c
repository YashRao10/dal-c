/*
 * test_sc_watchdog.c -- requirements-based tests for sc_watchdog.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_watchdog.h"

static const sc_watchdog_config_t CFG2 = { 2u };
static const sc_watchdog_config_t CFG3 = { 3u };

/* HLR-WDG-1 / LLR-WDG-1,2,3 -- config validation. Each condition of the
 * decision is shown to independently decide the outcome. */
static void wdg_config_validation(void)
{
    static const sc_watchdog_config_t zero = { 0u };
    static const sc_watchdog_config_t big  = { 1000u };

    SC_CHECK_EQ(sc_watchdog_config_valid(NULL),  SC_ERR_NULL);   /* cfg==NULL decides   */
    SC_CHECK_EQ(sc_watchdog_config_valid(&zero), SC_ERR_PARAM);  /* timeout==0 decides  */
    SC_CHECK_EQ(sc_watchdog_config_valid(&CFG3), SC_OK);         /* both false          */
    SC_CHECK_EQ(sc_watchdog_config_valid(&big),  SC_OK);
}

/* HLR-WDG-2 / LLR-WDG-4,5 -- init clears both fields; NULL rejected. */
static void wdg_init_and_null(void)
{
    sc_watchdog_state_t st;

    SC_CHECK_EQ(sc_watchdog_init(NULL), SC_ERR_NULL);

    st.elapsed = 7u;
    st.tripped = true;
    SC_MUST(sc_watchdog_init(&st));
    SC_CHECK_EQ(st.elapsed, 0u);
    SC_CHECK(st.tripped == false);
}

/* HLR-WDG-4 / LLR-WDG-8 -- a NULL cfg or state fails safe to TRIPPED and
 * does not touch the state. Covers both operands of the OR plus the
 * both-false case. */
static void wdg_tick_null_failsafe(void)
{
    sc_watchdog_state_t st;
    SC_MUST(sc_watchdog_init(&st));

    SC_CHECK_EQ(sc_watchdog_tick(NULL, &st), SC_WATCHDOG_TRIPPED);   /* cfg decides   */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, NULL), SC_WATCHDOG_TRIPPED); /* state decides */
    SC_CHECK_EQ(st.elapsed, 0u);                                     /* untouched     */
    SC_CHECK(st.tripped == false);

    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* both false     */
}

/* HLR-WDG-5 / LLR-WDG-9,10,11 -- with no kick, the watchdog trips exactly
 * on the tick at which the elapsed count reaches `timeout`. */
static void wdg_trips_at_timeout(void)
{
    sc_watchdog_state_t st;
    SC_MUST(sc_watchdog_init(&st));

    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* elapsed 0 -> 1 */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* elapsed 1 -> 2 */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_TRIPPED);  /* elapsed 2 -> 3 */
    SC_CHECK_EQ(st.elapsed, 3u);
    SC_CHECK(st.tripped == true);
}

/* HLR-WDG-3 / LLR-WDG-6,7 -- a kick resets the count, so the deadline is
 * measured afresh from the kick. */
static void wdg_kick_defers_trip(void)
{
    sc_watchdog_state_t st;
    SC_MUST(sc_watchdog_init(&st));

    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* elapsed -> 1 */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* elapsed -> 2 */

    sc_watchdog_kick(&st);
    SC_CHECK_EQ(st.elapsed, 0u);
    SC_CHECK(st.tripped == false);

    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* elapsed -> 1 */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG3, &st), SC_WATCHDOG_OK);       /* elapsed -> 2, still OK */
    SC_CHECK(sc_watchdog_expired(&st) == false);

    sc_watchdog_kick(NULL);                                         /* LLR-WDG-6: no effect */
    SC_CHECK_EQ(st.elapsed, 2u);
}

/* HLR-WDG-6 / LLR-WDG-10,12 -- once tripped, it stays tripped through both
 * an at-ceiling tick and a post-kick in-deadline tick; only init clears it. */
static void wdg_latches(void)
{
    sc_watchdog_state_t st;
    SC_MUST(sc_watchdog_init(&st));

    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_OK);       /* elapsed -> 1        */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_TRIPPED);  /* elapsed -> 2, trip  */

    /* elapsed already at the ceiling: the increment is skipped, verdict
     * still TRIPPED from the latched flag. */
    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_TRIPPED);
    SC_CHECK_EQ(st.elapsed, 2u);

    /* a kick clears the count but not the latch */
    sc_watchdog_kick(&st);
    SC_CHECK_EQ(st.elapsed, 0u);
    SC_CHECK(sc_watchdog_expired(&st) == true);
    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_TRIPPED);  /* elapsed 0 -> 1, still latched */

    /* only re-init clears it */
    SC_MUST(sc_watchdog_init(&st));
    SC_CHECK(sc_watchdog_expired(&st) == false);
    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_OK);
}

/* HLR-WDG-6 / LLR-WDG-12 -- the expired() accessor. Each operand of
 * (state != NULL) && (state->tripped) is shown to independently decide. */
static void wdg_expired_query(void)
{
    sc_watchdog_state_t st;

    SC_CHECK(sc_watchdog_expired(NULL) == false);        /* state==NULL decides       */

    SC_MUST(sc_watchdog_init(&st));
    SC_CHECK(sc_watchdog_expired(&st) == false);         /* non-NULL, not tripped     */

    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_OK);
    SC_CHECK_EQ(sc_watchdog_tick(&CFG2, &st), SC_WATCHDOG_TRIPPED);
    SC_CHECK(sc_watchdog_expired(&st) == true);          /* non-NULL, tripped         */
}

/* HLR-WDG-5 -- timeout == 1 is degenerate: the activity must kick before
 * every tick, and a kick alone cannot save it. */
static void wdg_timeout_one(void)
{
    static const sc_watchdog_config_t one = { 1u };
    sc_watchdog_state_t st;

    SC_MUST(sc_watchdog_init(&st));
    SC_CHECK_EQ(sc_watchdog_tick(&one, &st), SC_WATCHDOG_TRIPPED);   /* elapsed 0 -> 1 >= 1 */

    SC_MUST(sc_watchdog_init(&st));
    sc_watchdog_kick(&st);
    SC_CHECK_EQ(sc_watchdog_tick(&one, &st), SC_WATCHDOG_TRIPPED);
}

/* HLR-WDG-5 / LLR-WDG-9 -- past the deadline the elapsed count is pinned at
 * `timeout`; it never climbs further and never wraps, no matter how many
 * ticks pass without a kick. */
static void wdg_elapsed_pinned_at_ceiling(void)
{
    sc_watchdog_state_t st;
    unsigned k;

    SC_MUST(sc_watchdog_init(&st));
    for (k = 0u; k < 50u; k++)
    {
        (void)sc_watchdog_tick(&CFG2, &st);
    }
    SC_CHECK_EQ(st.elapsed, 2u);
    SC_CHECK(st.tripped == true);
}

const sc_test_case sc_suite_watchdog[] = {
    { "wdg_config_validation",         wdg_config_validation         },
    { "wdg_init_and_null",             wdg_init_and_null             },
    { "wdg_tick_null_failsafe",        wdg_tick_null_failsafe        },
    { "wdg_trips_at_timeout",          wdg_trips_at_timeout          },
    { "wdg_kick_defers_trip",          wdg_kick_defers_trip          },
    { "wdg_latches",                   wdg_latches                   },
    { "wdg_expired_query",             wdg_expired_query             },
    { "wdg_timeout_one",               wdg_timeout_one               },
    { "wdg_elapsed_pinned_at_ceiling", wdg_elapsed_pinned_at_ceiling }
};
const unsigned sc_suite_watchdog_count =
    (unsigned)(sizeof(sc_suite_watchdog) / sizeof(sc_suite_watchdog[0]));
