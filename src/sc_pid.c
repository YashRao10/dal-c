/*
 * sc_pid.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_pid.md.
 */
#include "sc_pid.h"

sc_status_t sc_pid_config_valid(const sc_pid_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                                /* LLR-PID-11 */
    }
    else if (cfg->out_min > cfg->out_max)
    {
        status = SC_ERR_PARAM;                               /* LLR-PID-12 */
    }
    else if ((cfg->d_filter < 0) || (cfg->d_filter >= SC_Q16_ONE))
    {
        status = SC_ERR_PARAM;                               /* LLR-PID-13 */
    }
    else
    {
        status = SC_OK;                                      /* LLR-PID-14 */
    }

    return status;
}

sc_status_t sc_pid_init(sc_pid_state_t *state,
                        sc_q16_t initial_output,
                        sc_q16_t initial_measurement)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                                /* LLR-PID-15 */
    }
    else
    {
        state->integrator       = initial_output;           /* LLR-PID-16 */
        state->prev_measurement  = initial_measurement;
        state->prev_derivative   = 0;
        status = SC_OK;
    }

    return status;
}

sc_q16_t sc_pid_update(const sc_pid_config_t *cfg,
                       sc_pid_state_t *state,
                       sc_q16_t setpoint,
                       sc_q16_t measurement)
{
    sc_q16_t result;

    if ((cfg == NULL) || (state == NULL))
    {
        result = 0;                                          /* LLR-PID-1 */
    }
    else
    {
        sc_q16_t error   = sc_q16_sub(setpoint, measurement);      /* LLR-PID-2 */
        sc_q16_t p_term  = sc_q16_mul(cfg->kp, error);             /* LLR-PID-3 */

        sc_q16_t d_meas  = sc_q16_sub(measurement, state->prev_measurement);
        sc_q16_t d_raw   = sc_q16_mul(cfg->kd, sc_q16_sub(0, d_meas)); /* LLR-PID-4 */
        sc_q16_t d_term  = sc_q16_add(
            sc_q16_mul(cfg->d_filter, state->prev_derivative),
            sc_q16_mul(sc_q16_sub(SC_Q16_ONE, cfg->d_filter), d_raw)); /* LLR-PID-5 */

        sc_q16_t i_new   = sc_q16_add(state->integrator,
                                      sc_q16_mul(cfg->ki, error));     /* LLR-PID-6 */

        sc_q16_t out_raw = sc_q16_add(sc_q16_add(p_term, i_new), d_term); /* LLR-PID-7 */

        if (out_raw > cfg->out_max)
        {
            result = cfg->out_max;                                     /* LLR-PID-8 */
            if (i_new <= state->integrator)      /* only integrate off the rail */
            {
                state->integrator = i_new;
            }
        }
        else if (out_raw < cfg->out_min)
        {
            result = cfg->out_min;                                     /* LLR-PID-9 */
            if (i_new >= state->integrator)
            {
                state->integrator = i_new;
            }
        }
        else
        {
            result = out_raw;                                          /* LLR-PID-10 */
            state->integrator = i_new;
        }

        state->prev_measurement = measurement;
        state->prev_derivative  = d_term;
    }

    return result;
}
