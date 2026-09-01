# sc_crc — Requirements

Bitwise cyclic redundancy checks. No lookup tables.

Verified by: `tests/test_sc_crc.c`

## High-Level Requirements

- **HLR-CRC-1** — Each function shall compute its named standard CRC with
  the conventional parameters, matching the published check value for the
  message `"123456789"`.
- **HLR-CRC-2** — Any single-bit change anywhere in the message shall
  change the returned CRC.
- **HLR-CRC-3** — A zero-length message shall return the CRC's defined
  initial/framed value.
- **HLR-CRC-4** — A NULL data pointer shall be treated as a zero-length
  message regardless of the length argument.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-CRC-1 | HLR-CRC-1, HLR-CRC-3 | Each function iterates over `len` message bytes, folding each into the register; a zero-length message runs the loop zero times. | all |
| LLR-CRC-2 | HLR-CRC-1, HLR-CRC-2 | For every byte, the register is advanced by exactly eight single-bit steps. | all |
| LLR-CRC-3 | HLR-CRC-1, HLR-CRC-2 | For the non-reflected CRCs, a step XORs the polynomial when the top register bit is set, otherwise it only shifts left. | `sc_crc8_smbus`, `sc_crc16_ccitt_false` |
| LLR-CRC-4 | HLR-CRC-1, HLR-CRC-3 | `sc_crc8_smbus` returns the register unmodified (init 0x00, poly 0x07, no final XOR). | `sc_crc8_smbus` |
| LLR-CRC-5 | HLR-CRC-1, HLR-CRC-3 | `sc_crc16_ccitt_false` returns the register unmodified (init 0xFFFF, poly 0x1021). | `sc_crc16_ccitt_false` |
| LLR-CRC-6 | HLR-CRC-1, HLR-CRC-2 | `sc_crc32` uses the reflected algorithm: a step XORs 0xEDB88320 when the low register bit is set, otherwise it only shifts right. | `sc_crc32` |
| LLR-CRC-7 | HLR-CRC-4 | A NULL `data` pointer forces `len` to 0 before the loop. | all |
| LLR-CRC-8 | HLR-CRC-1, HLR-CRC-3 | `sc_crc32` applies the final XOR with 0xFFFFFFFF before returning. | `sc_crc32` |
