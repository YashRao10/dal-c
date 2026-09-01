/*
 * test_sc_ringbuf.c -- requirements-based tests for sc_ringbuf.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_ringbuf.h"

/* HLR-RB-1 / LLR-RB-1,9,10 -- init binds storage and rejects bad args. */
static void rb_init_and_errors(void)
{
    uint8_t store[4];
    sc_ringbuf_t rb;

    SC_CHECK_EQ(sc_ringbuf_init(NULL, store, 4u), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ringbuf_init(&rb, NULL, 4u), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ringbuf_init(&rb, store, 0u), SC_ERR_PARAM);

    SC_CHECK_EQ(sc_ringbuf_init(&rb, store, 4u), SC_OK);
    SC_CHECK(sc_ringbuf_is_empty(&rb) == true);
    SC_CHECK(sc_ringbuf_is_full(&rb) == false);
    SC_CHECK_EQ(sc_ringbuf_count(&rb), 0u);
}

/* HLR-RB-2 / LLR-RB-5 -- bytes come out in the order they went in. */
static void rb_fifo_order(void)
{
    uint8_t store[8];
    sc_ringbuf_t rb;
    uint8_t got = 0u;
    uint8_t i;

    SC_MUST(sc_ringbuf_init(&rb, store, 8u));
    for (i = 1u; i <= 5u; i++)
    {
        SC_CHECK_EQ(sc_ringbuf_push(&rb, i), SC_OK);
    }
    SC_CHECK_EQ(sc_ringbuf_count(&rb), 5u);
    for (i = 1u; i <= 5u; i++)
    {
        SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);
        SC_CHECK_EQ(got, i);
    }
    SC_CHECK(sc_ringbuf_is_empty(&rb) == true);
}

/* HLR-RB-3 / LLR-RB-7 -- indices wrap: fill, partially drain, refill across
 * the physical end of the buffer, drain again in order. */
static void rb_wraps_around(void)
{
    uint8_t store[3];
    sc_ringbuf_t rb;
    uint8_t got = 0u;

    SC_MUST(sc_ringbuf_init(&rb, store, 3u));
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 10u), SC_OK);
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 20u), SC_OK);
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);  SC_CHECK_EQ(got, 10u);
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 30u), SC_OK);  /* head wraps to 0 */
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 40u), SC_OK);  /* now full        */
    SC_CHECK(sc_ringbuf_is_full(&rb) == true);
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);  SC_CHECK_EQ(got, 20u);
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);  SC_CHECK_EQ(got, 30u); /* tail wraps */
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);  SC_CHECK_EQ(got, 40u);
}

/* HLR-RB-4 / LLR-RB-6 -- push into a full buffer is rejected, data intact. */
static void rb_rejects_when_full(void)
{
    uint8_t store[2];
    sc_ringbuf_t rb;
    uint8_t got = 0u;

    SC_MUST(sc_ringbuf_init(&rb, store, 2u));
    SC_MUST(sc_ringbuf_push(&rb, 1u));
    SC_MUST(sc_ringbuf_push(&rb, 2u));
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 3u), SC_ERR_FULL);
    SC_CHECK_EQ(sc_ringbuf_count(&rb), 2u);
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);  SC_CHECK_EQ(got, 1u);
}

/* HLR-RB-5 / LLR-RB-8 -- pop from an empty buffer is rejected. */
static void rb_rejects_when_empty(void)
{
    uint8_t store[4];
    sc_ringbuf_t rb;
    uint8_t got = 0xAAu;

    SC_MUST(sc_ringbuf_init(&rb, store, 4u));
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_ERR_EMPTY);
    SC_CHECK_EQ(got, 0xAAu);  /* out untouched */
}

/* HLR-RB-6 / LLR-RB-12 -- reset drops contents, storage still usable. */
static void rb_reset(void)
{
    uint8_t store[4];
    sc_ringbuf_t rb;
    uint8_t got = 0u;

    SC_MUST(sc_ringbuf_init(&rb, store, 4u));
    SC_MUST(sc_ringbuf_push(&rb, 7u));
    SC_MUST(sc_ringbuf_push(&rb, 8u));
    SC_CHECK_EQ(sc_ringbuf_reset(&rb), SC_OK);
    SC_CHECK(sc_ringbuf_is_empty(&rb) == true);
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 9u), SC_OK);
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);
    SC_CHECK_EQ(got, 9u);
}

/* Capacity-1 edge: alternately full and empty. */
static void rb_capacity_one(void)
{
    uint8_t store[1];
    sc_ringbuf_t rb;
    uint8_t got = 0u;

    SC_MUST(sc_ringbuf_init(&rb, store, 1u));
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 42u), SC_OK);
    SC_CHECK(sc_ringbuf_is_full(&rb) == true);
    SC_CHECK_EQ(sc_ringbuf_push(&rb, 43u), SC_ERR_FULL);
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, &got), SC_OK);
    SC_CHECK_EQ(got, 42u);
    SC_CHECK(sc_ringbuf_is_empty(&rb) == true);
}

/* LLR-RB-9,11 -- every entry point is null-safe. */
static void rb_null_safe(void)
{
    uint8_t got = 0u;
    SC_CHECK_EQ(sc_ringbuf_push(NULL, 1u), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ringbuf_pop(NULL, &got), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ringbuf_reset(NULL), SC_ERR_NULL);
    SC_CHECK(sc_ringbuf_is_empty(NULL) == true);
    SC_CHECK(sc_ringbuf_is_full(NULL) == false);
    SC_CHECK_EQ(sc_ringbuf_count(NULL), 0u);
}

/* LLR-RB-9 -- pop with a null output pointer is rejected. */
static void rb_pop_null_out(void)
{
    uint8_t store[2];
    sc_ringbuf_t rb;
    SC_MUST(sc_ringbuf_init(&rb, store, 2u));
    SC_MUST(sc_ringbuf_push(&rb, 1u));
    SC_CHECK_EQ(sc_ringbuf_pop(&rb, NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_ringbuf_count(&rb), 1u);
}

const sc_test_case sc_suite_ringbuf[] = {
    { "rb_init_and_errors",   rb_init_and_errors   },
    { "rb_fifo_order",        rb_fifo_order        },
    { "rb_wraps_around",      rb_wraps_around      },
    { "rb_rejects_when_full", rb_rejects_when_full },
    { "rb_rejects_when_empty",rb_rejects_when_empty},
    { "rb_reset",             rb_reset             },
    { "rb_capacity_one",      rb_capacity_one      },
    { "rb_null_safe",         rb_null_safe         },
    { "rb_pop_null_out",      rb_pop_null_out      }
};
const unsigned sc_suite_ringbuf_count =
    (unsigned)(sizeof(sc_suite_ringbuf) / sizeof(sc_suite_ringbuf[0]));
