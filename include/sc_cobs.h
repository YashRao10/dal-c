/*
 * sc_cobs.h -- Consistent Overhead Byte Stuffing.
 *
 * Encodes a byte string so the result contains no 0x00 bytes, letting
 * 0x00 be used as an unambiguous frame delimiter on a serial link. The
 * overhead is one byte, plus one more per 254 bytes of payload.
 *
 * These functions operate on a single frame's worth of bytes in caller
 * buffers; they do not read or write the 0x00 delimiter itself. Pair with
 * sc_ringbuf to assemble frames from a stream.
 */
#ifndef DAL_C_SC_COBS_H
#define DAL_C_SC_COBS_H

#include "sc_common.h"

/* Worst-case encoded length for a `payload_len`-byte payload. */
uint32_t sc_cobs_max_encoded(uint32_t payload_len);

/*
 * Encode `in_len` bytes from `in` into `out` (capacity `out_cap`).
 * Returns the encoded length, or 0 if an argument is NULL or the output
 * would not fit. The encoded bytes contain no 0x00.
 */
uint32_t sc_cobs_encode(const uint8_t *in, uint32_t in_len,
                        uint8_t *out, uint32_t out_cap);

/*
 * Decode `in_len` encoded bytes from `in` into `out` (capacity `out_cap`).
 * Returns the decoded length, or 0 if an argument is NULL, the input is
 * malformed (a block length runs past the end), or the output would not
 * fit. `in` must not contain 0x00.
 */
uint32_t sc_cobs_decode(const uint8_t *in, uint32_t in_len,
                        uint8_t *out, uint32_t out_cap);

#endif /* DAL_C_SC_COBS_H */
