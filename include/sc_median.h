/*
 * sc_median.h -- sliding-window median filter for impulse-noise rejection.
 *
 * The output is the middle value of the most recent `length` samples.
 * Unlike a moving average, a single spike (or any run of spikes shorter
 * than half the window) cannot reach the output at all -- it is sorted to
 * an end of the window and discarded. Used to clean a noisy sensor reading
 * before it drives a comparator or a control loop.
 *
 * `length` is odd so the median is a single unambiguous sample, never the
 * mean of two. The window storage lives inside the state; there is no
 * dynamic allocation and the insertion sort is bounded by
 * SC_MEDIAN_MAX_WINDOW.
 */
#ifndef DAL_C_SC_MEDIAN_H
#define DAL_C_SC_MEDIAN_H

#include "sc_common.h"

/* Largest window the filter will accept; sizes the in-state storage. */
#define SC_MEDIAN_MAX_WINDOW 15

typedef struct
{
    uint8_t length;   /* window width: odd, 1 .. SC_MEDIAN_MAX_WINDOW */
} sc_median_config_t;

typedef struct
{
    int32_t history[SC_MEDIAN_MAX_WINDOW];   /* most recent samples, ring */
    uint8_t count;                           /* live samples, 0 .. length */
    uint8_t head;                            /* next write slot            */
} sc_median_state_t;

/*
 * Validate a configuration.
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM length is 0, even, or > SC_MEDIAN_MAX_WINDOW
 */
SC_NODISCARD sc_status_t sc_median_config_valid(const sc_median_config_t *cfg);

/*
 * Initialise state to the empty window.
 *   SC_OK        done
 *   SC_ERR_NULL  state is NULL
 */
SC_NODISCARD sc_status_t sc_median_init(sc_median_state_t *state);

/*
 * Feed one raw sample and return the filtered output.
 *
 * Preconditions: `cfg` and `state` are non-NULL and `cfg` has passed
 * sc_median_config_valid(); the hot path does not repeat that check. If
 * either pointer is NULL the function returns 0 without dereferencing it.
 *
 * Until `length` samples have been seen the return value is the median of
 * the samples so far (of an even count, the higher of the two central
 * samples).
 */
int32_t sc_median_update(const sc_median_config_t *cfg,
                         sc_median_state_t *state,
                         int32_t sample);

#endif /* DAL_C_SC_MEDIAN_H */
