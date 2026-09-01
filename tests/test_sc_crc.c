/*
 * test_sc_crc.c -- requirements-based tests for sc_crc.
 *
 * Known-answer vectors from the standard CRC catalogue, plus the empty and
 * NULL cases. The "123456789" vector exercises the polynomial-XOR and
 * shift-only branches of every inner loop.
 */
#include <string.h>

#include "sc_test.h"
#include "sc_suites.h"
#include "sc_crc.h"

static const uint8_t CHECK_MSG[9] = { '1','2','3','4','5','6','7','8','9' };

/* HLR-CRC-1 -- catalogue check values for "123456789". */
static void crc_check_vectors(void)
{
    SC_CHECK_EQ(sc_crc8_smbus(CHECK_MSG, 9u), 0xF4u);
    SC_CHECK_EQ(sc_crc16_ccitt_false(CHECK_MSG, 9u), 0x29B1u);
    SC_CHECK_EQ(sc_crc32(CHECK_MSG, 9u), 0xCBF43926u);
}

/* HLR-CRC-2 -- a single-byte change changes the CRC (bit sensitivity). */
static void crc_detects_single_bit(void)
{
    uint8_t bad[9];
    (void)memcpy(bad, CHECK_MSG, sizeof bad);
    bad[4] ^= 0x01u;
    SC_CHECK(sc_crc8_smbus(bad, 9u) != 0xF4u);
    SC_CHECK(sc_crc16_ccitt_false(bad, 9u) != 0x29B1u);
    SC_CHECK(sc_crc32(bad, 9u) != 0xCBF43926u);
}

/* HLR-CRC-3 -- the empty message returns each CRC's initial/framed value. */
static void crc_empty_message(void)
{
    static const uint8_t one = 0x00u;
    SC_CHECK_EQ(sc_crc8_smbus(&one, 0u), 0x00u);
    SC_CHECK_EQ(sc_crc16_ccitt_false(&one, 0u), 0xFFFFu);
    SC_CHECK_EQ(sc_crc32(&one, 0u), 0x00000000u);
}

/* HLR-CRC-4 / LLR-CRC-7 -- a NULL pointer is treated as a zero-length
 * message (even if a non-zero length is passed). */
static void crc_null_data(void)
{
    SC_CHECK_EQ(sc_crc8_smbus(NULL, 0u), 0x00u);
    SC_CHECK_EQ(sc_crc8_smbus(NULL, 16u), 0x00u);
    SC_CHECK_EQ(sc_crc16_ccitt_false(NULL, 16u), 0xFFFFu);
    SC_CHECK_EQ(sc_crc32(NULL, 16u), 0x00000000u);
}

/* HLR-CRC-2 -- a byte of 0x00 still advances the register (shift-only path
 * on the first bit); a byte of 0xFF forces the polynomial-XOR path. */
static void crc_zero_and_ff_bytes(void)
{
    static const uint8_t zero = 0x00u;
    static const uint8_t ff   = 0xFFu;
    SC_CHECK_EQ(sc_crc8_smbus(&zero, 1u), 0x00u);
    SC_CHECK(sc_crc8_smbus(&ff, 1u) != 0x00u);
    SC_CHECK(sc_crc32(&ff, 1u) != 0x00000000u);
}

const sc_test_case sc_suite_crc[] = {
    { "crc_check_vectors",        crc_check_vectors        },
    { "crc_detects_single_bit",   crc_detects_single_bit   },
    { "crc_empty_message",        crc_empty_message        },
    { "crc_null_data",            crc_null_data            },
    { "crc_zero_and_ff_bytes",    crc_zero_and_ff_bytes    }
};
const unsigned sc_suite_crc_count =
    (unsigned)(sizeof(sc_suite_crc) / sizeof(sc_suite_crc[0]));
