/*
 * control_loop.c -- a worked example composing five dal-c components.
 *
 * A simulated room-temperature loop:
 *
 *   sensor --> sc_ringbuf (3-sample buffer) --> mean
 *                                                 |
 *                          sc_hysteresis <--------+--------> sc_pid --> sc_ratelimit --> heater
 *                          (heat-demand lamp)                (power cmd)  (slew limit)
 *
 * plus sc_crc32 to checksum a status frame each step. Prints a trace and
 * exits 0; nothing here is a test, it just shows the parts fitting together.
 */
#include <stdio.h>

#include "dal_c.h"

#define ONE       SC_Q16_ONE
#define DEG(whole, tenths)  ((sc_q16_t)(((whole) * ONE) + (((tenths) * ONE) / 10)))

/* every dal-c call that returns a status is checked */
#define MUST(call)                                                           \
    do {                                                                    \
        if ((call) != SC_OK) {                                              \
            (void)printf("dal-c call failed: %s\n", #call);                 \
            return 1;                                                       \
        }                                                                  \
    } while (0)

int main(void)
{
    /* --- configuration ------------------------------------------------ */
    const sc_q16_t setpoint = DEG(21, 0);

    sc_pid_config_t pid_cfg;
    sc_ratelimit_config_t slew_cfg;
    sc_hysteresis_config_t lamp_cfg;

    sc_pid_state_t       pid;
    sc_ratelimit_state_t slew;
    sc_hysteresis_state_t lamp;

    sc_ringbuf_t rb;
    uint8_t rb_store[3];

    sc_q16_t room = DEG(17, 0);   /* cold start */
    sc_q16_t loss = ONE / 20;     /* heat loss per step toward ambient */
    sc_q16_t ambient = DEG(15, 0);
    sc_q16_t gain = ONE / 6;      /* room heating per unit of heater power */
    int step;

    pid_cfg.kp = ONE * 3;
    pid_cfg.ki = ONE / 8;
    pid_cfg.kd = ONE / 2;
    pid_cfg.d_filter = ONE / 3;
    pid_cfg.out_min = 0;
    pid_cfg.out_max = ONE * 10;   /* heater power 0..10 */

    slew_cfg.max_step_up = ONE * 2;
    slew_cfg.max_step_down = ONE * 2;
    slew_cfg.out_min = 0;
    slew_cfg.out_max = ONE * 10;

    lamp_cfg.low_threshold  = -(ONE / 5);   /* lamp clears within 0.2 deg */
    lamp_cfg.high_threshold = -(ONE / 50);  /* lamp asserts when >0.02 below setpoint */

    if ((sc_pid_config_valid(&pid_cfg) != SC_OK) ||
        (sc_ratelimit_config_valid(&slew_cfg) != SC_OK) ||
        (sc_hysteresis_config_valid(&lamp_cfg) != SC_OK))
    {
        (void)printf("bad config\n");
        return 1;
    }

    MUST(sc_pid_init(&pid, 0, room));
    MUST(sc_ratelimit_init(&slew, 0));
    MUST(sc_hysteresis_init(&lamp));
    MUST(sc_ringbuf_init(&rb, rb_store, (uint32_t)sizeof rb_store));

    (void)printf("step   room   mean  power  heat  frame-crc\n");

    for (step = 0; step < 24; step++)
    {
        /* sensor sample (integer degrees for the byte buffer) into the ring */
        uint8_t sample = (uint8_t)sc_q16_to_int(room);
        sc_q16_t mean;
        sc_q16_t sum = 0;
        uint32_t n = 0u;
        uint8_t v;
        uint8_t frame[4];
        uint32_t crc;
        sc_q16_t demand;
        sc_q16_t raw_power;
        sc_q16_t power;
        int lamp_on;

        if (sc_ringbuf_is_full(&rb))
        {
            MUST(sc_ringbuf_pop(&rb, &v));
        }
        MUST(sc_ringbuf_push(&rb, sample));

        /* mean of whatever the buffer holds (1..3 samples) */
        {
            sc_ringbuf_t scan = rb;
            while (sc_ringbuf_pop(&scan, &v) == SC_OK)
            {
                sum = sc_q16_add(sum, sc_q16_from_int((int32_t)v));
                n++;
            }
        }
        mean = sc_q16_div(sum, sc_q16_from_int((int32_t)n));

        /* heat-demand lamp: how far below setpoint is the mean? */
        demand = sc_q16_sub(mean, setpoint);
        lamp_on = sc_hysteresis_update(&lamp_cfg, &lamp, demand) ? 1 : 0;

        /* PID on the filtered measurement, then slew-limit the command */
        raw_power = sc_pid_update(&pid_cfg, &pid, setpoint, mean);
        power = sc_ratelimit_update(&slew_cfg, &slew, raw_power);

        /* status frame + CRC */
        frame[0] = (uint8_t)sc_q16_to_int(room);
        frame[1] = (uint8_t)sc_q16_to_int(power);
        frame[2] = (uint8_t)lamp_on;
        frame[3] = (uint8_t)step;
        crc = sc_crc32(frame, (uint32_t)sizeof frame);

        (void)printf("%3d   %5d  %5d  %5d   %s   %08lx\n",
                     step,
                     sc_q16_to_int(sc_q16_mul(room, sc_q16_from_int(10))),
                     sc_q16_to_int(sc_q16_mul(mean, sc_q16_from_int(10))),
                     sc_q16_to_int(sc_q16_mul(power, sc_q16_from_int(10))),
                     lamp_on ? "ON " : "off",
                     (unsigned long)crc);

        /* plant: heater raises the room, ambient pulls it down */
        room = sc_q16_add(room, sc_q16_mul(gain, power));
        room = sc_q16_sub(room, sc_q16_mul(loss, sc_q16_sub(room, ambient)));
    }

    (void)printf("\nsettled near setpoint %d.%d C\n",
                 sc_q16_to_int(room),
                 sc_q16_to_int(sc_q16_mul(sc_q16_abs(sc_q16_sub(room,
                     sc_q16_from_int(sc_q16_to_int(room)))), sc_q16_from_int(10))));
    return 0;
}
