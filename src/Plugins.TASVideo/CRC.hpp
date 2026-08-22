/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void CRC_BuildTable();

uint32_t CRC_Calculate(uint32_t crc, void *buffer, uint32_t count);
uint32_t CRC_CalculatePalette(uint32_t crc, void *buffer, uint32_t count);
