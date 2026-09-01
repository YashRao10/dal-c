/*
 * sc_sm.h -- table-driven finite state machine engine.
 *
 * The caller supplies a flat transition table: rows of
 * (from-state, event, to-state, optional action). The engine holds only
 * the current state id. `sc_sm_dispatch` scans the table for the first row
 * matching the current state and the delivered event, runs its action (if
 * any), and moves to the destination state. An event with no matching row
 * is ignored.
 *
 * No dynamic memory; the scan is a bounded linear pass over the table.
 * State and event ids are opaque uint16_t values chosen by the caller.
 */
#ifndef DAL_C_SC_SM_H
#define DAL_C_SC_SM_H

#include "sc_common.h"

/* Transition action. `ctx` is the opaque pointer passed to sc_sm_dispatch. */
typedef void (*sc_sm_action)(void *ctx);

typedef struct
{
    uint16_t     from;    /* source state id      */
    uint16_t     event;   /* triggering event id  */
    uint16_t     to;      /* destination state id */
    sc_sm_action action;  /* run on transition, or NULL */
} sc_sm_transition;

typedef struct
{
    const sc_sm_transition *table;
    uint16_t                n;        /* number of rows, >= 1 */
    uint16_t                initial;  /* starting state id     */
} sc_sm_config_t;

typedef struct
{
    uint16_t state;
} sc_sm_state_t;

/*
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg or cfg->table is NULL
 *   SC_ERR_PARAM n == 0
 */
SC_NODISCARD sc_status_t sc_sm_config_valid(const sc_sm_config_t *cfg);

/* Set the machine to `cfg->initial`. SC_OK / SC_ERR_NULL. */
SC_NODISCARD sc_status_t sc_sm_init(sc_sm_state_t *state, const sc_sm_config_t *cfg);

/* Current state id, or 0xFFFF if `state` is NULL. */
uint16_t sc_sm_state(const sc_sm_state_t *state);

/*
 * Deliver `event`. Returns true if a transition fired (and the state has
 * moved / the action has run), false if no row matched or an argument was
 * NULL. Assumes `cfg` has passed sc_sm_config_valid().
 */
bool sc_sm_dispatch(const sc_sm_config_t *cfg,
                    sc_sm_state_t *state,
                    uint16_t event,
                    void *ctx);

#endif /* DAL_C_SC_SM_H */
