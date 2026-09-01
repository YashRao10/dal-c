/*
 * sc_sched.h -- cooperative cyclic scheduler.
 *
 * A fixed table of tasks, each with a period and a phase offset in ticks.
 * The application calls sc_sched_tick() once per timer interrupt (or main-
 * loop pass); the scheduler runs every task that is due on that tick, in
 * table order, to completion. No preemption, no dynamic memory, no
 * priorities beyond table order.
 *
 * A task runs on tick T when  (T % period) == phase.
 */
#ifndef DAL_C_SC_SCHED_H
#define DAL_C_SC_SCHED_H

#include "sc_common.h"

typedef void (*sc_sched_task)(void *ctx);

typedef struct
{
    sc_sched_task run;     /* task entry point                       */
    uint32_t      period;  /* run every `period` ticks, >= 1          */
    uint32_t      phase;   /* offset of the first run, < period       */
} sc_sched_slot;

typedef struct
{
    const sc_sched_slot *slots;
    uint16_t             n;   /* number of slots, >= 1 */
} sc_sched_config_t;

typedef struct
{
    uint32_t tick;
} sc_sched_state_t;

/*
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg, cfg->slots, or any slot's run pointer is NULL
 *   SC_ERR_PARAM n == 0, or a slot has period 0 or phase >= period
 */
SC_NODISCARD sc_status_t sc_sched_config_valid(const sc_sched_config_t *cfg);

/* Reset the tick counter to 0. SC_OK / SC_ERR_NULL. */
SC_NODISCARD sc_status_t sc_sched_init(sc_sched_state_t *state);

/*
 * Run every task due on the current tick (table order), then advance the
 * tick. Returns the number of tasks run. A NULL cfg or state returns 0
 * and does not advance. Assumes cfg has passed sc_sched_config_valid().
 */
uint32_t sc_sched_tick(const sc_sched_config_t *cfg,
                       sc_sched_state_t *state,
                       void *ctx);

#endif /* DAL_C_SC_SCHED_H */
