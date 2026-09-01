/*
 * sc_pid.h -- positional PID controller, fixed-point, with anti-windup.
 *
 * One update per control cycle. Everything is Q16.16 (see sc_fixed.h) and
 * saturating, so no term can wrap. The controller has:
 *
 *   - proportional, integral and derivative action;
 *   - derivative taken on the measurement, not the error, so a setpoint
 *     step does not produce a derivative spike ("derivative kick");
 *   - a first-order low-pass on the derivative term;
 *   - output clamping to [out_min, out_max];
 *   - conditional-integration anti-windup: while the output is on a rail,
 *     the integrator is only updated in the direction that moves the
 *     output off that rail.
 *
 * The sample period is folded into the gains: `ki` is the integral gain
 * per cycle (Ki * dt) and `kd` is the derivative gain per cycle (Kd / dt).
 */
#ifndef DAL_C_SC_PID_H
#define DAL_C_SC_PID_H

#include "sc_common.h"
#include "sc_fixed.h"

typedef struct
{
    sc_q16_t kp;        /* proportional gain                              */
    sc_q16_t ki;        /* integral gain per cycle (Ki * dt)             */
    sc_q16_t kd;        /* derivative gain per cycle (Kd / dt)           */
    sc_q16_t d_filter;  /* derivative low-pass coeff, [0, SC_Q16_ONE)     */
    sc_q16_t out_min;
    sc_q16_t out_max;
} sc_pid_config_t;

typedef struct
{
    sc_q16_t integrator;
    sc_q16_t prev_measurement;
    sc_q16_t prev_derivative;
} sc_pid_state_t;

/*
 * Validate a configuration.
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM out_min > out_max, or d_filter outside [0, SC_Q16_ONE)
 */
SC_NODISCARD sc_status_t sc_pid_config_valid(const sc_pid_config_t *cfg);

/*
 * Initialise state for a bumpless start. The integrator is preloaded with
 * `initial_output` so that, with the loop near steady state, the first
 * update returns close to that value; `initial_measurement` seeds the
 * derivative history.
 *   SC_OK / SC_ERR_NULL
 */
SC_NODISCARD sc_status_t sc_pid_init(sc_pid_state_t *state,
                                     sc_q16_t initial_output,
                                     sc_q16_t initial_measurement);

/*
 * Run one control cycle and return the clamped controller output.
 *
 * Preconditions: `cfg` and `state` are non-NULL and `cfg` has passed
 * sc_pid_config_valid(). A NULL `state` returns 0; a NULL `cfg` with a
 * non-NULL `state` returns 0 without modifying the state.
 */
sc_q16_t sc_pid_update(const sc_pid_config_t *cfg,
                       sc_pid_state_t *state,
                       sc_q16_t setpoint,
                       sc_q16_t measurement);

#endif /* DAL_C_SC_PID_H */
