/*
 * sc_cobs.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_cobs.md.
 *
 * Encode uses "lazy" block splitting: a full 254-byte block is only
 * closed with a 0xFF code when there is a following byte, so a payload of
 * exactly 254 non-zero bytes encodes to 255 bytes, not 256.
 */
#include "sc_cobs.h"

#define COBS_FULL_BLOCK  0xFFu   /* code value meaning "254 data bytes, no trailing zero" */

uint32_t sc_cobs_max_encoded(uint32_t payload_len)
{
    uint32_t result;

    if (payload_len == 0u)
    {
        result = 1u;                                     /* LLR-COBS-1 */
    }
    else
    {
        result = payload_len + ((payload_len - 1u) / 254u) + 1u;
    }

    return result;
}

uint32_t sc_cobs_encode(const uint8_t *in, uint32_t in_len,
                        uint8_t *out, uint32_t out_cap)
{
    uint32_t result = 0u;

    if ((in == NULL) || (out == NULL) ||
        (sc_cobs_max_encoded(in_len) > out_cap))
    {
        result = 0u;                                     /* LLR-COBS-2 */
    }
    else
    {
        uint32_t rd = 0u;
        uint32_t wr = 1u;      /* out[0] reserved for the first code */
        uint32_t code_at = 0u;
        uint8_t  code = 1u;

        while (rd < in_len)                               /* LLR-COBS-3 */
        {
            if (code == COBS_FULL_BLOCK)                  /* LLR-COBS-6: close a full block */
            {
                out[code_at] = code;
                code_at = wr;
                wr++;
                code = 1u;
            }

            if (in[rd] == 0u)
            {
                out[code_at] = code;                     /* LLR-COBS-4: close block on zero */
                code_at = wr;
                wr++;
                code = 1u;
            }
            else
            {
                out[wr] = in[rd];                        /* LLR-COBS-5: copy non-zero */
                wr++;
                code = (uint8_t)(code + 1u);
            }
            rd++;
        }

        out[code_at] = code;                             /* LLR-COBS-7: final code */
        result = wr;
    }

    return result;
}

uint32_t sc_cobs_decode(const uint8_t *in, uint32_t in_len,
                        uint8_t *out, uint32_t out_cap)
{
    uint32_t result = 0u;

    if ((in == NULL) || (out == NULL))
    {
        result = 0u;                                     /* LLR-COBS-8 */
    }
    else
    {
        uint32_t rd = 0u;
        uint32_t wr = 0u;
        bool     ok = true;

        while ((rd < in_len) && ok)                       /* LLR-COBS-9 */
        {
            uint8_t code = in[rd];

            rd++;

            if (code == 0u)
            {
                ok = false;                              /* LLR-COBS-10: 0x00 is not a valid code */
            }
            else
            {
                uint32_t span = (uint32_t)code - 1u;

                if ((span > (in_len - rd)) || (span > (out_cap - wr)))
                {
                    ok = false;                          /* malformed / no room */
                }
                else
                {
                    uint32_t k;
                    for (k = 0u; k < span; k++)           /* LLR-COBS-11: copy block */
                    {
                        out[wr] = in[rd];
                        wr++;
                        rd++;
                    }
                    if ((code != COBS_FULL_BLOCK) && (rd < in_len))
                    {
                        if (wr >= out_cap)
                        {
                            ok = false;
                        }
                        else
                        {
                            out[wr] = 0u;                /* LLR-COBS-12: inter-block zero */
                            wr++;
                        }
                    }
                }
            }
        }

        result = ok ? wr : 0u;                            /* LLR-COBS-13 */
    }

    return result;
}
