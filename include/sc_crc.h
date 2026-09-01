/*
 * sc_crc.h -- cyclic redundancy checks for message integrity.
 *
 * Bitwise (no lookup tables) so the whole computation is inspectable and
 * the working set is a few bytes. Each function computes a standard,
 * named CRC with its conventional parameters baked in; the check value
 * for the ASCII string "123456789" is given for each.
 *
 * A NULL `data` pointer is treated as a zero-length message.
 */
#ifndef DAL_C_SC_CRC_H
#define DAL_C_SC_CRC_H

#include "sc_common.h"

/* CRC-8/SMBUS: poly 0x07, init 0x00, no reflection, no final XOR.
 * check("123456789") == 0xF4 */
uint8_t sc_crc8_smbus(const uint8_t *data, uint32_t len);

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final XOR.
 * check("123456789") == 0x29B1 */
uint16_t sc_crc16_ccitt_false(const uint8_t *data, uint32_t len);

/* CRC-32/ISO-HDLC (zlib): poly 0x04C11DB7 reflected, init 0xFFFFFFFF,
 * input and output reflected, final XOR 0xFFFFFFFF.
 * check("123456789") == 0xCBF43926 */
uint32_t sc_crc32(const uint8_t *data, uint32_t len);

#endif /* DAL_C_SC_CRC_H */
