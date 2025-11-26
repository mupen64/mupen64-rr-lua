/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "audiohle.h"

void ADDMIXER()
{
    s16 Count = k0 >> 12 & 0x0FF0;
    u16 InBuffer = t9 >> 16;
    u16 OutBuffer = t9 & 0xffff;

    s16 *inp, *outp;
    s32 temp;
    inp = (s16*)(BufferSpace + InBuffer);
    outp = (s16*)(BufferSpace + OutBuffer);
    for (s16 cntr = 0; cntr < Count; cntr += 2)
    {
        temp = *outp + *inp;
        *outp = pack_signed(temp);
        outp++;
        inp++;
    }
}

void HILOGAIN()
{
    u16 cnt = k0 & 0xffff;
    u16 out = t9 >> 16 & 0xffff;
    s16 hi = (s16)(k0 >> 4 & 0xf000);
    u16 lo = k0 >> 20 & 0xf;
    s16* src;

    src = (s16*)(BufferSpace + out);
    s32 tmp, val;

    while (cnt)
    {
        val = (s32)*src;
        // tmp = ((val * (s32)hi) + ((u64)(val * lo) << 16) >> 16);
        tmp = ((val * (s32)hi) >> 16) + (u32)(val * lo);
        *src = pack_signed(tmp);
        src++;
        cnt -= 2;
    }
}

void INTERLEAVE()
{
    u32 inL, inR;
    u16* outbuff = (u16*)(AudioOutBuffer + BufferSpace);
    u16* inSrcR;
    u16* inSrcL;
    u16 Left, Right;

    inL = t9 & 0xFFFF;
    inR = t9 >> 16 & 0xFFFF;

    inSrcR = (u16*)(BufferSpace + inR);
    inSrcL = (u16*)(BufferSpace + inL);

    for (int x = 0; x < AudioCount / 4; x++)
    {
        Left = *inSrcL++;
        Right = *inSrcR++;

        *outbuff++ = *inSrcR++;
        *outbuff++ = *inSrcL++;
        *outbuff++ = (u16)Right;
        *outbuff++ = (u16)Left;
    }
}

void INTERL2()
{
    s16 Count = k0 & 0xFFFF;
    u16 Out = t9 & 0xffff;
    u16 In = t9 >> 16;
    u8* src;
    u8* dst;

    src = &BufferSpace[0]; //[In];
    dst = &BufferSpace[0]; //[Out];
    while (Count != 0)
    {
        *(s16*)(dst + BES(Out)) = *(s16*)(src + BES(In));
        Out += 2;
        In += 4;
        Count--;
    }
}

void INTERLEAVE2()
{ // Needs accuracy verification...
    u32 inL, inR;
    u16* outbuff;
    u16* inSrcR;
    u16* inSrcL;
    u16 Left, Right;
    u32 count;
    count = k0 >> 12 & 0xFF0;
    if (count == 0)
    {
        outbuff = (u16*)(AudioOutBuffer + BufferSpace);
        count = AudioCount;
    }
    else
    {
        outbuff = (u16*)((k0 & 0xFFFF) + BufferSpace);
    }

    inR = t9 & 0xFFFF;
    inL = t9 >> 16 & 0xFFFF;

    inSrcR = (u16*)(BufferSpace + inR);
    inSrcL = (u16*)(BufferSpace + inL);

    for (u32 x = 0; x < count / 4; x++)
    {
        Left = *inSrcL++;
        Right = *inSrcR++;

        *outbuff++ = *inSrcR++;
        *outbuff++ = *inSrcL++;
        *outbuff++ = (u16)Right;
        *outbuff++ = (u16)Left;
    }
}

void INTERLEAVE3()
{ // Needs accuracy verification...
    // u32 inL, inR;
    u16* outbuff = (u16*)(BufferSpace + 0x4f0); //(u16 *)(AudioOutBuffer+dmem);
    u16* inSrcR;
    u16* inSrcL;
    u16 Left, Right;

    // inR = t9 & 0xFFFF;
    // inL = (t9 >> 16) & 0xFFFF;

    inSrcR = (u16*)(BufferSpace + 0xb40);
    inSrcL = (u16*)(BufferSpace + 0x9d0);

    for (int x = 0; x < 0x170 / 4; x++)
    {
        Left = *inSrcL++;
        Right = *inSrcR++;

        *outbuff++ = *inSrcR++;
        *outbuff++ = *inSrcL++;
        *outbuff++ = (u16)Right;
        *outbuff++ = (u16)Left;
        /*
        Left=*(inSrcL++);
        Right=*(inSrcR++);
        *(outbuff++)=(u16)Left;
        Left >>= 16;
        *(outbuff++)=(u16)Right;
        Right >>= 16;
        *(outbuff++)=(u16)Left;
        *(outbuff++)=(u16)Right;*/
    }
}

void MIXER()
{
    u32 dmemin = (u16)(t9 >> 0x10);
    u32 dmemout = (u16)(t9 & 0xFFFF);
    // u8  flags = (u8)((k0 >> 16) & 0xff);
    s32 gain = (s16)(k0 & 0xFFFF);
    s32 temp;

    if (AudioCount == 0)
        return;

    for (int x = 0; x < AudioCount; x += 2)
    {
        temp = (*(s16*)(BufferSpace + dmemin + x) * gain) >> 15;
        temp += *(s16*)(BufferSpace + dmemout + x);

        *(s16*)(BufferSpace + dmemout + x) = pack_signed(temp);
    }
}

void MIXER2()
{ // Needs accuracy verification...
    u16 dmemin = (u16)(t9 >> 0x10);
    u16 dmemout = (u16)(t9 & 0xFFFF);
    u32 count = k0 >> 12 & 0xFF0;
    s32 gain = (s16)(k0 & 0xFFFF) * 2;
    s32 temp;

    for (u32 x = 0; x < count; x += 2)
    { // I think I can do this a lot easier

        temp = (*(s16*)(BufferSpace + dmemin + x) * gain) >> 16;
        temp += *(s16*)(BufferSpace + dmemout + x);

        *(s16*)(BufferSpace + dmemout + x) = pack_signed(temp);
    }
}

void MIXER3()
{ // Needs accuracy verification...
    u16 dmemin = (u16)(t9 >> 0x10) + 0x4f0;
    u16 dmemout = (u16)(t9 & 0xFFFF) + 0x4f0;
    // u8  flags = (u8)((k0 >> 16) & 0xff);
    s32 gain = (s16)(k0 & 0xFFFF) * 2;
    s32 temp;

    for (int x = 0; x < 0x170; x += 2)
    { // I think I can do this a lot easier
        temp = (*(s16*)(BufferSpace + dmemin + x) * gain) >> 16;
        temp += *(s16*)(BufferSpace + dmemout + x);

        *(s16*)(BufferSpace + dmemout + x) = pack_signed(temp);
    }
}
