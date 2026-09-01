/*
 * test_sc_cobs.c -- requirements-based tests for sc_cobs.
 */
#include <string.h>

#include "sc_test.h"
#include "sc_suites.h"
#include "sc_cobs.h"

static int eq(const uint8_t *a, const uint8_t *b, uint32_t n)
{
    return memcmp(a, b, (size_t)n) == 0;
}

/* HLR-COBS-1 / LLR-COBS-1 -- worst-case size formula. */
static void cobs_max_size(void)
{
    SC_CHECK_EQ(sc_cobs_max_encoded(0u), 1u);
    SC_CHECK_EQ(sc_cobs_max_encoded(1u), 2u);
    SC_CHECK_EQ(sc_cobs_max_encoded(254u), 255u);
    SC_CHECK_EQ(sc_cobs_max_encoded(255u), 257u);
}

/* HLR-COBS-2 -- known encodings from the COBS specification. */
static void cobs_known_vectors(void)
{
    uint8_t out[16];
    static const uint8_t p1[1] = { 0x00 };
    static const uint8_t e1[2] = { 0x01, 0x01 };
    static const uint8_t p2[4] = { 0x11, 0x22, 0x00, 0x33 };
    static const uint8_t e2[5] = { 0x03, 0x11, 0x22, 0x02, 0x33 };
    static const uint8_t p3[4] = { 0x11, 0x22, 0x33, 0x44 };
    static const uint8_t e3[5] = { 0x05, 0x11, 0x22, 0x33, 0x44 };
    static const uint8_t p4[4] = { 0x11, 0x00, 0x00, 0x00 };
    static const uint8_t e4[5] = { 0x02, 0x11, 0x01, 0x01, 0x01 };

    SC_CHECK_EQ(sc_cobs_encode(p1, 0u, out, sizeof out), 1u);   /* empty -> 0x01 */
    SC_CHECK_EQ(out[0], 0x01u);

    SC_CHECK_EQ(sc_cobs_encode(p1, 1u, out, sizeof out), 2u); SC_CHECK(eq(out, e1, 2u));
    SC_CHECK_EQ(sc_cobs_encode(p2, 4u, out, sizeof out), 5u); SC_CHECK(eq(out, e2, 5u));
    SC_CHECK_EQ(sc_cobs_encode(p3, 4u, out, sizeof out), 5u); SC_CHECK(eq(out, e3, 5u));
    SC_CHECK_EQ(sc_cobs_encode(p4, 4u, out, sizeof out), 5u); SC_CHECK(eq(out, e4, 5u));
}

/* HLR-COBS-3 -- decode is the exact inverse of encode. */
static void cobs_round_trip(void)
{
    uint8_t payload[600];
    uint8_t enc[700];
    uint8_t dec[600];
    uint32_t sizes[6] = { 0u, 1u, 5u, 253u, 254u, 512u };
    unsigned s;
    uint32_t i;

    for (s = 0u; s < 6u; s++)
    {
        uint32_t n = sizes[s];
        for (i = 0u; i < n; i++)
        {
            payload[i] = (uint8_t)((i * 7u) & 0xFFu);   /* includes zeros */
        }
        {
            uint32_t el = sc_cobs_encode(payload, n, enc, sizeof enc);
            uint32_t dl;
            SC_CHECK(el > 0u);
            /* encoded frame must contain no 0x00 */
            for (i = 0u; i < el; i++)
            {
                SC_CHECK(enc[i] != 0u);
            }
            dl = sc_cobs_decode(enc, el, dec, sizeof dec);
            SC_CHECK_EQ(dl, n);
            SC_CHECK(eq(dec, payload, n));
        }
    }
}

/* HLR-COBS-3 -- the 254-byte block boundary (code 0xFF, no trailing zero). */
static void cobs_block_boundary(void)
{
    uint8_t payload[300];
    uint8_t enc[320];
    uint8_t dec[300];
    uint32_t i;

    for (i = 0u; i < 300u; i++)
    {
        payload[i] = 0xABu;   /* all non-zero -> forces a 0xFF block */
    }
    /* exactly 254 */
    SC_CHECK_EQ(sc_cobs_encode(payload, 254u, enc, sizeof enc), 255u);
    SC_CHECK_EQ(enc[0], 0xFFu);
    SC_CHECK_EQ(sc_cobs_decode(enc, 255u, dec, sizeof dec), 254u);
    SC_CHECK(eq(dec, payload, 254u));
    /* 255 -> a full block then one more */
    {
        uint32_t el = sc_cobs_encode(payload, 255u, enc, sizeof enc);
        SC_CHECK_EQ(el, 257u);
        SC_CHECK_EQ(sc_cobs_decode(enc, el, dec, sizeof dec), 255u);
        SC_CHECK(eq(dec, payload, 255u));
    }
}

/* HLR-COBS-2 / LLR-COBS-2,8 -- bad arguments and tight buffers. */
static void cobs_bad_args(void)
{
    uint8_t in[4]  = { 1, 2, 3, 4 };
    uint8_t out[8];
    SC_CHECK_EQ(sc_cobs_encode(NULL, 4u, out, sizeof out), 0u);
    SC_CHECK_EQ(sc_cobs_encode(in, 4u, NULL, sizeof out), 0u);
    SC_CHECK_EQ(sc_cobs_encode(in, 4u, out, 4u), 0u);    /* needs 5 */
    SC_CHECK_EQ(sc_cobs_decode(NULL, 4u, out, sizeof out), 0u);
    SC_CHECK_EQ(sc_cobs_decode(in, 4u, NULL, sizeof out), 0u);
}

/* HLR-COBS-4 / LLR-COBS-10 -- malformed encoded input is rejected. */
static void cobs_malformed_decode(void)
{
    uint8_t out[8];
    static const uint8_t runs_past[2] = { 0x05, 0x11 };       /* code 5 but 1 byte left */
    static const uint8_t bad_code[2]  = { 0x01, 0x00 };       /* 0x00 where a code is expected */

    SC_CHECK_EQ(sc_cobs_decode(runs_past, 2u, out, sizeof out), 0u);
    SC_CHECK_EQ(sc_cobs_decode(bad_code, 2u, out, sizeof out), 0u);

    /* decode into a buffer too small for the block itself */
    {
        uint8_t enc[8];
        uint8_t in[4] = { 9, 8, 7, 6 };
        uint32_t el = sc_cobs_encode(in, 4u, enc, sizeof enc);
        SC_CHECK_EQ(sc_cobs_decode(enc, el, out, 3u), 0u);
    }
    /* buffer holds the data but not the inter-block separator zero */
    {
        uint8_t enc[8];
        uint8_t in[3] = { 9, 0, 8 };
        uint32_t el = sc_cobs_encode(in, 3u, enc, sizeof enc);
        SC_CHECK_EQ(sc_cobs_decode(enc, el, out, 1u), 0u);
    }
}

const sc_test_case sc_suite_frame[] = {
    { "cobs_max_size",         cobs_max_size         },
    { "cobs_known_vectors",    cobs_known_vectors    },
    { "cobs_round_trip",       cobs_round_trip       },
    { "cobs_block_boundary",   cobs_block_boundary   },
    { "cobs_bad_args",         cobs_bad_args         },
    { "cobs_malformed_decode", cobs_malformed_decode }
};
const unsigned sc_suite_frame_count =
    (unsigned)(sizeof(sc_suite_frame) / sizeof(sc_suite_frame[0]));
