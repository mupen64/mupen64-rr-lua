/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

struct DepthBuffer
{
    DepthBuffer *higher, *lower;

    u32 address, cleared;
};

struct DepthBufferInfo
{
    DepthBuffer *top, *bottom, *current;
    int numBuffers;
};

extern DepthBufferInfo depthBuffer;

void DepthBuffer_Init();
void DepthBuffer_Destroy();
void DepthBuffer_SetBuffer(u32 address);
void DepthBuffer_RemoveBuffer(u32 address);
DepthBuffer *DepthBuffer_FindBuffer(u32 address);