/*
 * sc_crc.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_crc.md.
 */
#include "sc_crc.h"

uint8_t sc_crc8_smbus(const uint8_t *data, uint32_t len)
{
    uint8_t  crc = 0x00u;
    uint32_t i;
    uint8_t  bit;

    if (data == NULL)
    {
        len = 0u;                                   /* LLR-CRC-7 */
    }

    for (i = 0u; i < len; i++)                       /* LLR-CRC-1 */
    {
        crc ^= data[i];
        for (bit = 0u; bit < 8u; bit++)             /* LLR-CRC-2 */
        {
            if ((crc & 0x80u) != 0u)                /* LLR-CRC-3 */
            {
                crc = (uint8_t)((uint8_t)(crc << 1) ^ 0x07u);
            }
            else
            {
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    return crc;                                      /* LLR-CRC-4 */
}

uint16_t sc_crc16_ccitt_false(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFu;
    uint32_t i;
    uint8_t  bit;

    if (data == NULL)
    {
        len = 0u;                                   /* LLR-CRC-7 */
    }

    for (i = 0u; i < len; i++)                       /* LLR-CRC-1 */
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (bit = 0u; bit < 8u; bit++)             /* LLR-CRC-2 */
        {
            if ((crc & 0x8000u) != 0u)              /* LLR-CRC-3 */
            {
                crc = (uint16_t)((uint16_t)(crc << 1) ^ 0x1021u);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;                                      /* LLR-CRC-5 */
}

uint32_t sc_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    uint8_t  bit;

    if (data == NULL)
    {
        len = 0u;                                   /* LLR-CRC-7 */
    }

    for (i = 0u; i < len; i++)                       /* LLR-CRC-1 */
    {
        crc ^= (uint32_t)data[i];
        for (bit = 0u; bit < 8u; bit++)             /* LLR-CRC-2 */
        {
            if ((crc & 0x00000001u) != 0u)          /* LLR-CRC-6: reflected */
            {
                crc = (crc >> 1) ^ 0xEDB88320u;
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }

    return crc ^ 0xFFFFFFFFu;                        /* LLR-CRC-8: final XOR */
}
