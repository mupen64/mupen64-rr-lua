/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define F3DEX3_TRISNAKE 0x08
#define F3DEX3_RELSEGMENT 0x0B

#define F3DEX3_MV_VIEWPORT 8
#define F3DEX3_MV_LIGHT 10

#define F3DEX3_MW_FX 0x00

void F3DEX3_Mtx(u32 w0, u32 w1);
void F3DEX3_MoveMem(u32 w0, u32 w1);
void F3DEX3_MoveWord(u32 w0, u32 w1);
void F3DEX3_TriSnake(u32 w0, u32 w1);
void F3DEX3_RelSegment(u32 w0, u32 w1);
void F3DEX3_Init();
