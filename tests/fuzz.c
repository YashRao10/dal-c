/*
 * fuzz.c -- randomised property / invariant harness.
 *
 * Not requirements-based: this drives components with pseudo-random input
 * and asserts invariants that must hold for *every* input, catching classes
 * of bug that example-based tests can miss. Deterministic (fixed seed) so a
 * failure reproduces. Run it under the sanitizers for the strongest signal:
 *   make fuzz            (plain)
 *   make sanitize        (also builds + runs this under UBSan/ASan)
 */
#include <stdio.h>
#include <stdlib.h>

#include "dal_c.h"

#define ITERS 200000u

static uint32_t rng_state = 0x1234abcdu;

static uint32_t rnd(void)
{
    /* xorshift32 */
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static int32_t rnd_i32(void) { return (int32_t)rnd(); }

static unsigned failures = 0u;

#define INV(cond)                                                          \
    do {                                                                  \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            (void)printf("  INVARIANT FAILED  %s:%d  %s  (state 0x%08x)\n",\
                         __FILE__, __LINE__, #cond, rng_state);           \
        }                                                                 \
    } while (0)

/* sc_sat: never wraps; equals the clamped true sum in 64-bit. */
static void fuzz_sat(void)
{
    int32_t a = rnd_i32();
    int32_t b = rnd_i32();
    int64_t t = (int64_t)a + (int64_t)b;
    int32_t r = sc_sat_add_i32(a, b);
    if (t > (int64_t)INT32_MAX)      { INV(r == INT32_MAX); }
    else if (t < (int64_t)INT32_MIN) { INV(r == INT32_MIN); }
    else                             { INV(r == (int32_t)t); }
}

/* sc_ratelimit: output always in range and never slews faster than allowed. */
static void fuzz_ratelimit(void)
{
    sc_ratelimit_config_t cfg;
    sc_ratelimit_state_t st;
    int32_t prev;
    unsigned k;

    cfg.max_step_up   = (int32_t)(rnd() % 5000u);
    cfg.max_step_down = (int32_t)(rnd() % 5000u);
    cfg.out_min = -(int32_t)(rnd() % 100000u);
    cfg.out_max =  (int32_t)(rnd() % 100000u);
    if (cfg.out_min > cfg.out_max) { return; }
    if (sc_ratelimit_init(&st, 0) != SC_OK) { return; }

    prev = 0;
    for (k = 0u; k < 20u; k++)
    {
        int32_t target = rnd_i32();
        int32_t now = sc_ratelimit_update(&cfg, &st, target);
        int64_t delta = (int64_t)now - (int64_t)prev;
        INV(now >= cfg.out_min);
        INV(now <= cfg.out_max);
        /* step bound holds unless a clamp pulled it further (first move only) */
        if ((prev >= cfg.out_min) && (prev <= cfg.out_max))
        {
            INV(delta <= (int64_t)cfg.max_step_up);
            INV(delta >= -(int64_t)cfg.max_step_down);
        }
        prev = now;
    }
}

/* sc_pid: output is always clamped to [out_min, out_max]. */
static void fuzz_pid(void)
{
    sc_pid_config_t cfg;
    sc_pid_state_t st;
    unsigned k;

    cfg.kp = (sc_q16_t)(rnd() % 0x40000u);
    cfg.ki = (sc_q16_t)(rnd() % 0x4000u);
    cfg.kd = (sc_q16_t)(rnd() % 0x20000u);
    cfg.d_filter = (sc_q16_t)(rnd() % (uint32_t)SC_Q16_ONE);
    cfg.out_min = -(int32_t)(rnd() % 0x200000u);
    cfg.out_max =  (int32_t)(rnd() % 0x200000u);
    if (sc_pid_config_valid(&cfg) != SC_OK) { return; }
    if (sc_pid_init(&st, 0, 0) != SC_OK) { return; }

    for (k = 0u; k < 30u; k++)
    {
        sc_q16_t sp = rnd_i32();
        sc_q16_t meas = rnd_i32();
        sc_q16_t u = sc_pid_update(&cfg, &st, sp, meas);
        INV(u >= cfg.out_min);
        INV(u <= cfg.out_max);
    }
}

/* sc_ringbuf: FIFO order preserved; count within [0, capacity]. */
static void fuzz_ringbuf(void)
{
    uint8_t store[64];
    uint8_t model[64];
    uint32_t head = 0u, tail = 0u, count = 0u;
    sc_ringbuf_t rb;
    uint32_t cap = 1u + (rnd() % 64u);
    unsigned k;

    if (sc_ringbuf_init(&rb, store, cap) != SC_OK) { return; }

    for (k = 0u; k < 400u; k++)
    {
        INV(sc_ringbuf_count(&rb) == count);
        INV(sc_ringbuf_count(&rb) <= cap);
        if ((rnd() & 1u) != 0u)
        {
            uint8_t v = (uint8_t)rnd();
            sc_status_t s = sc_ringbuf_push(&rb, v);
            if (count < cap) { INV(s == SC_OK); model[head] = v; head = (head + 1u) % cap; count++; }
            else             { INV(s == SC_ERR_FULL); }
        }
        else
        {
            uint8_t got = 0u;
            sc_status_t s = sc_ringbuf_pop(&rb, &got);
            if (count > 0u) { INV(s == SC_OK); INV(got == model[tail]); tail = (tail + 1u) % cap; count--; }
            else            { INV(s == SC_ERR_EMPTY); }
        }
    }
}

/* sc_cobs: round-trips exactly; encoded bytes are never 0x00. */
static void fuzz_cobs(void)
{
    uint8_t in[300];
    uint8_t enc[400];
    uint8_t dec[300];
    uint32_t n = rnd() % 300u;
    uint32_t i, el, dl;

    for (i = 0u; i < n; i++) { in[i] = (uint8_t)rnd(); }

    el = sc_cobs_encode(in, n, enc, sizeof enc);
    INV(el > 0u);
    for (i = 0u; i < el; i++) { INV(enc[i] != 0u); }
    dl = sc_cobs_decode(enc, el, dec, sizeof dec);
    INV(dl == n);
    for (i = 0u; i < n; i++) { INV(dec[i] == in[i]); }
}

/* sc_hysteresis: above the high threshold the output is asserted, below the
 * low threshold it is cleared, and strictly between it holds its value. */
static void fuzz_hysteresis(void)
{
    sc_hysteresis_config_t cfg;
    sc_hysteresis_state_t st;
    int prev;
    unsigned k;

    cfg.low_threshold  = rnd_i32() / 2;
    cfg.high_threshold = cfg.low_threshold + (int32_t)(rnd() % 100000u);
    if (sc_hysteresis_config_valid(&cfg) != SC_OK) { return; }
    if (sc_hysteresis_init(&st) != SC_OK) { return; }

    prev = 0;
    for (k = 0u; k < 40u; k++)
    {
        int32_t in = rnd_i32();
        int now = sc_hysteresis_update(&cfg, &st, in) ? 1 : 0;
        if (in >= cfg.high_threshold)     { INV(now == 1); }
        else if (in <= cfg.low_threshold) { INV(now == 0); }
        else                              { INV(now == prev); }
        prev = now;
    }
}

int main(void)
{
    uint32_t i;
    for (i = 0u; i < ITERS; i++)
    {
        fuzz_sat();
        fuzz_ratelimit();
        fuzz_pid();
        fuzz_ringbuf();
        fuzz_cobs();
        fuzz_hysteresis();
    }
    (void)printf("%u iterations, %u invariant failure(s)\n", ITERS, failures);
    return (failures == 0u) ? EXIT_SUCCESS : EXIT_FAILURE;
}
