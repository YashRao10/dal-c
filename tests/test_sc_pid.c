/*
 * test_sc_pid.c -- requirements-based tests for sc_pid.
 *
 * Mixes closed-loop behaviour against a simple simulated plant with
 * single-step tests that force each branch of the clamp / anti-windup
 * logic.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_pid.h"

#define ONE SC_Q16_ONE

/* first-order plant: y += a * (u - y) */
static sc_q16_t plant_step(sc_q16_t y, sc_q16_t u, sc_q16_t a)
{
    return sc_q16_add(y, sc_q16_mul(a, sc_q16_sub(u, y)));
}

/* HLR-PID-1 / LLR-PID-11..14 -- config validation. */
static void pid_config_validation(void)
{
    static const sc_pid_config_t good      = { ONE, 0, 0, 0, -ONE, ONE };
    static const sc_pid_config_t bad_clamp  = { ONE, 0, 0, 0, ONE, -ONE };
    static const sc_pid_config_t bad_filt_hi = { ONE, 0, 0, ONE, -ONE, ONE };
    static const sc_pid_config_t bad_filt_lo = { ONE, 0, 0, -1, -ONE, ONE };

    SC_CHECK_EQ(sc_pid_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_pid_config_valid(&bad_clamp), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_pid_config_valid(&bad_filt_hi), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_pid_config_valid(&bad_filt_lo), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_pid_config_valid(&good), SC_OK);
}

/* HLR-PID-2 / LLR-PID-15,16 -- init and null-safety. */
static void pid_init_and_null(void)
{
    static const sc_pid_config_t cfg = { ONE, 0, 0, 0, -100 * ONE, 100 * ONE };
    sc_pid_state_t st;

    SC_CHECK_EQ(sc_pid_init(NULL, 0, 0), SC_ERR_NULL);
    SC_CHECK_EQ(sc_pid_init(&st, 7 * ONE, 2 * ONE), SC_OK);
    SC_CHECK_EQ(st.integrator, 7 * ONE);
    SC_CHECK_EQ(st.prev_measurement, 2 * ONE);
    SC_CHECK_EQ(st.prev_derivative, 0);

    SC_CHECK_EQ(sc_pid_update(NULL, &st, ONE, 0), 0);
    SC_CHECK_EQ(sc_pid_update(&cfg, NULL, ONE, 0), 0);
}

/* HLR-PID-3 / LLR-PID-3 -- proportional-only response. */
static void pid_p_only(void)
{
    static const sc_pid_config_t cfg = { 2 * ONE, 0, 0, 0, -100 * ONE, 100 * ONE };
    sc_pid_state_t st;
    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    SC_CHECK_EQ(sc_pid_update(&cfg, &st, ONE, 0), 2 * ONE);      /* 2 * error */
    SC_CHECK_EQ(sc_pid_update(&cfg, &st, ONE, ONE), 0);          /* error 0 */
}

/* HLR-PID-4 / LLR-PID-6 -- integral action accumulates each cycle. */
static void pid_i_ramps(void)
{
    static const sc_pid_config_t cfg = { 0, ONE / 2, 0, 0, -100 * ONE, 100 * ONE };
    sc_pid_state_t st;
    sc_q16_t a, b, c;
    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    a = sc_pid_update(&cfg, &st, ONE, 0);
    b = sc_pid_update(&cfg, &st, ONE, 0);
    c = sc_pid_update(&cfg, &st, ONE, 0);
    SC_CHECK(b > a);
    SC_CHECK(c > b);
}

/* HLR-PID-5 / LLR-PID-4 -- derivative acts on the measurement, so a
 * setpoint step alone produces no derivative contribution. */
static void pid_no_derivative_kick(void)
{
    static const sc_pid_config_t cfg = { 0, 0, 10 * ONE, 0, -1000 * ONE, 1000 * ONE };
    sc_pid_state_t st;
    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    /* setpoint jumps, measurement has not moved: D term is 0 */
    SC_CHECK_EQ(sc_pid_update(&cfg, &st, 100 * ONE, 0), 0);
    /* now the measurement moves: D term responds (negative, opposing) */
    SC_CHECK(sc_pid_update(&cfg, &st, 100 * ONE, ONE) < 0);
}

/* HLR-PID-6 / LLR-PID-8,9,10 -- output is clamped to [out_min, out_max]. */
static void pid_output_clamped(void)
{
    static const sc_pid_config_t cfg = { 10 * ONE, 0, 0, 0, -5 * ONE, 5 * ONE };
    sc_pid_state_t st;
    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    SC_CHECK_EQ(sc_pid_update(&cfg, &st, 100 * ONE, 0), 5 * ONE);      /* hi rail */
    SC_CHECK_EQ(sc_pid_update(&cfg, &st, -100 * ONE, 0), -5 * ONE);    /* lo rail */
    SC_CHECK_EQ(sc_pid_update(&cfg, &st, 0, 0), 0);                    /* in range */
}

/* HLR-PID-7 / LLR-PID-8,9 -- anti-windup: while on a rail the integrator
 * only moves in the direction that leaves the rail. */
static void pid_anti_windup(void)
{
    static const sc_pid_config_t cfg = { ONE, ONE, 0, 0, -5 * ONE, 5 * ONE };
    sc_pid_state_t st;

    /* high rail, error positive -> would wind up -> integrator frozen */
    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    (void)sc_pid_update(&cfg, &st, 100 * ONE, 0);
    SC_CHECK_EQ(st.integrator, 0);          /* not integrated */

    /* integrator preloaded past the rail, measurement now overshoots the
     * setpoint -> error negative -> integrator allowed to unwind */
    SC_CHECK_EQ(sc_pid_init(&st, 50 * ONE, 0), SC_OK);
    (void)sc_pid_update(&cfg, &st, 0, 3 * ONE);
    SC_CHECK(st.integrator < 50 * ONE);     /* unwound toward the band */

    /* low rail, error negative -> would wind down -> integrator frozen */
    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    (void)sc_pid_update(&cfg, &st, -100 * ONE, 0);
    SC_CHECK_EQ(st.integrator, 0);

    /* low rail, error positive -> integrator allowed to recover */
    SC_CHECK_EQ(sc_pid_init(&st, -50 * ONE, 0), SC_OK);
    (void)sc_pid_update(&cfg, &st, 0, -3 * ONE);
    SC_CHECK(st.integrator > -50 * ONE);
}

/* HLR-PID-8 -- closed loop converges onto the setpoint and the command
 * never leaves the actuator limits. */
static void pid_closed_loop_converges(void)
{
    static const sc_pid_config_t cfg =
        { ONE / 3, ONE / 40, ONE / 10, ONE / 4, -20 * ONE, 20 * ONE };
    sc_pid_state_t st;
    sc_q16_t y = 0;
    sc_q16_t sp = 10 * ONE;
    sc_q16_t a = ONE / 8;               /* plant time constant */
    int k;

    SC_CHECK_EQ(sc_pid_init(&st, 0, 0), SC_OK);
    for (k = 0; k < 400; k++)
    {
        sc_q16_t u = sc_pid_update(&cfg, &st, sp, y);
        SC_CHECK(u <= 20 * ONE);
        SC_CHECK(u >= -20 * ONE);
        y = plant_step(y, u, a);
    }
    SC_CHECK(sc_q16_abs(sc_q16_sub(y, sp)) < (ONE / 4));   /* within 0.25 */
}

const sc_test_case sc_suite_pid[] = {
    { "pid_config_validation",       pid_config_validation       },
    { "pid_init_and_null",           pid_init_and_null           },
    { "pid_p_only",                  pid_p_only                  },
    { "pid_i_ramps",                 pid_i_ramps                 },
    { "pid_no_derivative_kick",      pid_no_derivative_kick      },
    { "pid_output_clamped",          pid_output_clamped          },
    { "pid_anti_windup",             pid_anti_windup             },
    { "pid_closed_loop_converges",   pid_closed_loop_converges   }
};
const unsigned sc_suite_pid_count =
    (unsigned)(sizeof(sc_suite_pid) / sizeof(sc_suite_pid[0]));
