/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"

#define CRC32_POLYNOMIAL 0x04C11DB7

unsigned long CRCTable[256];

uint32_t Reflect(uint32_t ref, char ch)
{
    uint32_t value = 0;

    // Swap bit 0 for bit 7
    // bit 1 for bit 6, etc.
    for (int i = 1; i < (ch + 1); i++)
    {
        if (ref & 1) value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

void CRC_BuildTable()
{
    uint32_t crc;

    for (int i = 0; i <= 255; i++)
    {
        crc = Reflect(i, 8) << 24;
        for (int j = 0; j < 8; j++) crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);

        CRCTable[i] = Reflect(crc, 32);
    }
}

uint32_t CRC_Calculate(uint32_t crc, void *buffer, int32_t count)
{
    uint8_t *p;
    uint32_t orig = crc;

    p = (uint8_t *)buffer;
    while (count--) crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

    return crc ^ orig;
}

uint32_t CRC_CalculatePalette(uint32_t crc, void *buffer, int32_t count)
{
    uint8_t *p;
    uint32_t orig = crc;

    p = (uint8_t *)buffer;
    while (count--)
    {
        crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
        crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

        p += 6;
    }

    return crc ^ orig;
}
