/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* memset() and memcpy() */
#include <string.h>

#include "audiohle.h"

void CLEARBUFF()
{
    u32 addr = (u32)(k0 & 0xffff);
    u32 count = (u32)(t9 & 0xffff);
    addr &= 0xFFFC;
    memset(BufferSpace + addr, 0, count + 3 & 0xFFFC);
}

void CLEARBUFF2()
{
    u16 addr = (u16)(k0 & 0xffff);
    u16 count = (u16)(t9 & 0xffff);
    if (count > 0)
        memset(BufferSpace + addr, 0, count);
}

void CLEARBUFF3()
{
    u16 addr = (u16)(k0 & 0xffff);
    u16 count = (u16)(t9 & 0xffff);
    memset(BufferSpace + addr + 0x4f0, 0, count);
}

void DMEMMOVE()
{
    u32 v0, v1;
    u32 cnt;
    if ((t9 & 0xffff) == 0)
        return;
    v0 = k0 & 0xFFFF;
    v1 = t9 >> 0x10;

    u32 count = t9 + 3 & 0xfffc;

    for (cnt = 0; cnt < count; cnt += 4)
    {
        BufferSpace[BES(v1 + cnt + 0)] = BufferSpace[BES(v0 + cnt + 0)];
        BufferSpace[BES(v1 + cnt + 1)] = BufferSpace[BES(v0 + cnt + 1)];
        BufferSpace[BES(v1 + cnt + 2)] = BufferSpace[BES(v0 + cnt + 2)];
        BufferSpace[BES(v1 + cnt + 3)] = BufferSpace[BES(v0 + cnt + 3)];
    }
}

void DMEMMOVE2()
{ // Needs accuracy verification...
    u32 v0, v1;
    u32 cnt;
    if ((t9 & 0xffff) == 0)
        return;
    v0 = k0 & 0xFFFF;
    v1 = t9 >> 0x10;
    // assert ((v1 & 0x3) == 0);
    // assert ((v0 & 0x3) == 0);
    u32 count = t9 + 3 & 0xfffc;
    // v0 = (v0) & 0xfffc;
    // v1 = (v1) & 0xfffc;

    // memcpy (dmem+v1, dmem+v0, count-1);
    for (cnt = 0; cnt < count; cnt += 4)
    {
        BufferSpace[BES(v1 + cnt + 0)] = BufferSpace[BES(v0 + cnt + 0)];
        BufferSpace[BES(v1 + cnt + 1)] = BufferSpace[BES(v0 + cnt + 1)];
        BufferSpace[BES(v1 + cnt + 2)] = BufferSpace[BES(v0 + cnt + 2)];
        BufferSpace[BES(v1 + cnt + 3)] = BufferSpace[BES(v0 + cnt + 3)];
    }
}

void DMEMMOVE3()
{ // Needs accuracy verification...
    u32 v0, v1;
    u32 cnt;
    v0 = (k0 & 0xFFFF) + 0x4f0;
    v1 = (t9 >> 0x10) + 0x4f0;
    u32 count = t9 + 3 & 0xfffc;

    // memcpy (dmem+v1, dmem+v0, count-1);
    for (cnt = 0; cnt < count; cnt += 4)
    {
        BufferSpace[BES(v1 + cnt + 0)] = BufferSpace[BES(v0 + cnt + 0)];
        BufferSpace[BES(v1 + cnt + 1)] = BufferSpace[BES(v0 + cnt + 1)];
        BufferSpace[BES(v1 + cnt + 2)] = BufferSpace[BES(v0 + cnt + 2)];
        BufferSpace[BES(v1 + cnt + 3)] = BufferSpace[BES(v0 + cnt + 3)];
    }
}

void DUPLICATE2()
{
    u16 Count = k0 >> 16 & 0xff;
    u16 In = k0 & 0xffff;
    u16 Out = t9 >> 16;

    u16 buff[64];

    memcpy(buff, BufferSpace + In, 128);

    while (Count)
    {
        memcpy(BufferSpace + Out, buff, 128);
        Out += 128;
        Count--;
    }
}

// TODO: This comment has me wondering if there's a problem.  10+ year old comments are hard to remember. -Azimer
void LOADBUFF()
{ // memcpy causes static... endianess issue :(
    u32 v0;
    if (AudioCount == 0)
        return;
    v0 = t9 & 0xfffffc; // + SEGMENTS[(t9>>24)&0xf];
    memcpy(BufferSpace + (AudioInBuffer & 0xFFFC), DRAM + v0, AudioCount + 3 & 0xFFFC);
}

void LOADBUFF2()
{ // Needs accuracy verification...
    u32 v0;
    u32 cnt = (k0 >> 0xC) + 3 & 0xFFC;
    v0 = t9 & 0xfffffc; // + SEGMENTS[(t9>>24)&0xf];
    memcpy(BufferSpace + (k0 & 0xfffc), DRAM + v0, cnt + 3 & 0xFFFC);
}

void LOADBUFF3()
{
    u32 v0;
    u32 cnt = (k0 >> 0xC) + 3 & 0xFFC;
    v0 = t9 & 0xfffffc;
    u32 src = (k0 & 0xffc) + 0x4f0;
    memcpy(BufferSpace + src, DRAM + v0, cnt);
}

// TODO: This comment has me wondering if there's a problem.  10+ year old comments are hard to remember. -Azimer
void SAVEBUFF()
{ // memcpy causes static... endianess issue :(
    u32 v0;
    if (AudioCount == 0)
        return;
    v0 = t9 & 0xfffffc; // + SEGMENTS[(t9>>24)&0xf];
    memcpy(DRAM + v0, BufferSpace + (AudioOutBuffer & 0xFFFC), AudioCount + 3 & 0xFFFC);
}

void SAVEBUFF2()
{ // Needs accuracy verification...
    u32 v0;
    u32 cnt = (k0 >> 0xC) + 3 & 0xFFC;
    v0 = t9 & 0xfffffc; // + SEGMENTS[(t9>>24)&0xf];
    memcpy(DRAM + v0, BufferSpace + (k0 & 0xfffc), cnt + 3 & 0xFFFC);
}

void SAVEBUFF3()
{
    u32 v0;
    u32 cnt = (k0 >> 0xC) + 3 & 0xFFC;
    v0 = t9 & 0xfffffc;
    u32 src = (k0 & 0xffc) + 0x4f0;
    memcpy(DRAM + v0, BufferSpace + src, cnt);
}

void SEGMENT()
{ // Should work
    SEGMENTS[t9 >> 24 & 0xf] = t9 & 0xffffff;
}

void SEGMENT2()
{
    if (isZeldaABI)
    {
        FILTER2();
        return;
    }
    if ((k0 & 0xffffff) == 0)
    {
        isMKABI = true;
        // SEGMENTS[(t9>>24)&0xf] = (t9 & 0xffffff);
    }
    else
    {
        isMKABI = false;
        isZeldaABI = true;
        FILTER2();
    }
}

void SETBUFF()
{ // Should work ;-)
    if (k0 >> 0x10 & 0x8)
    { // A_AUX - Auxillary Sound Buffer Settings
        AudioAuxA = (u16)(k0 & 0xFFFF);
        AudioAuxC = (u16)(t9 >> 0x10);
        AudioAuxE = (u16)(t9 & 0xFFFF);
    }
    else
    { // A_MAIN - Main Sound Buffer Settings
        AudioInBuffer = (u16)(k0 & 0xFFFF); // 0x00
        AudioOutBuffer = (u16)(t9 >> 0x10); // 0x02
        AudioCount = (u16)(t9 & 0xFFFF); // 0x04
    }
}

void SETBUFF2()
{
    AudioInBuffer = (u16)(k0 & 0xFFFF); // 0x00
    AudioOutBuffer = (u16)(t9 >> 0x10); // 0x02
    AudioCount = (u16)(t9 & 0xFFFF); // 0x04
}

void SETLOOP()
{
    loopval = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    // VolTrg_Left  = (s16)(loopval>>16);		// m_LeftVol
    // VolRamp_Left = (s16)(loopval);	// m_LeftVolTarget
}

void SETLOOP2()
{
    loopval = t9 & 0xffffff; // No segment?
}

void SETLOOP3()
{
    loopval = t9 & 0xffffff;
}
