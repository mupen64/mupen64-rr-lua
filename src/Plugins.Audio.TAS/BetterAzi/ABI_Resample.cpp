/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "audiohle.h"
#include <cassert>

u16 ResampleLUT[0x200] = {
0x0C39, 0x66AD, 0x0D46, 0xFFDF, 0x0B39, 0x6696, 0x0E5F, 0xFFD8, 0x0A44, 0x6669, 0x0F83, 0xFFD0, 0x095A, 0x6626, 0x10B4, 0xFFC8, 0x087D, 0x65CD, 0x11F0, 0xFFBF, 0x07AB, 0x655E, 0x1338, 0xFFB6, 0x06E4, 0x64D9, 0x148C, 0xFFAC, 0x0628, 0x643F, 0x15EB, 0xFFA1, 0x0577, 0x638F, 0x1756, 0xFF96, 0x04D1, 0x62CB, 0x18CB, 0xFF8A, 0x0435, 0x61F3, 0x1A4C, 0xFF7E, 0x03A4, 0x6106, 0x1BD7, 0xFF71, 0x031C, 0x6007, 0x1D6C, 0xFF64, 0x029F, 0x5EF5, 0x1F0B, 0xFF56, 0x022A, 0x5DD0, 0x20B3, 0xFF48, 0x01BE, 0x5C9A, 0x2264, 0xFF3A, 0x015B, 0x5B53, 0x241E, 0xFF2C, 0x0101, 0x59FC, 0x25E0, 0xFF1E, 0x00AE, 0x5896, 0x27A9, 0xFF10, 0x0063, 0x5720, 0x297A, 0xFF02, 0x001F, 0x559D, 0x2B50, 0xFEF4, 0xFFE2, 0x540D, 0x2D2C, 0xFEE8, 0xFFAC, 0x5270, 0x2F0D, 0xFEDB, 0xFF7C, 0x50C7, 0x30F3, 0xFED0, 0xFF53, 0x4F14, 0x32DC, 0xFEC6, 0xFF2E, 0x4D57, 0x34C8, 0xFEBD, 0xFF0F, 0x4B91, 0x36B6, 0xFEB6, 0xFEF5, 0x49C2, 0x38A5, 0xFEB0, 0xFEDF, 0x47ED, 0x3A95, 0xFEAC, 0xFECE, 0x4611, 0x3C85, 0xFEAB, 0xFEC0, 0x4430, 0x3E74, 0xFEAC, 0xFEB6, 0x424A, 0x4060, 0xFEAF, 0xFEAF, 0x4060, 0x424A, 0xFEB6, 0xFEAC, 0x3E74, 0x4430, 0xFEC0, 0xFEAB, 0x3C85, 0x4611, 0xFECE, 0xFEAC, 0x3A95, 0x47ED, 0xFEDF, 0xFEB0, 0x38A5, 0x49C2, 0xFEF5, 0xFEB6, 0x36B6, 0x4B91, 0xFF0F, 0xFEBD, 0x34C8, 0x4D57, 0xFF2E, 0xFEC6, 0x32DC, 0x4F14, 0xFF53, 0xFED0, 0x30F3, 0x50C7, 0xFF7C, 0xFEDB, 0x2F0D, 0x5270, 0xFFAC, 0xFEE8, 0x2D2C, 0x540D, 0xFFE2, 0xFEF4, 0x2B50, 0x559D, 0x001F, 0xFF02, 0x297A, 0x5720, 0x0063, 0xFF10, 0x27A9, 0x5896, 0x00AE, 0xFF1E, 0x25E0, 0x59FC, 0x0101, 0xFF2C, 0x241E, 0x5B53, 0x015B, 0xFF3A, 0x2264, 0x5C9A, 0x01BE, 0xFF48, 0x20B3, 0x5DD0, 0x022A, 0xFF56, 0x1F0B, 0x5EF5, 0x029F, 0xFF64, 0x1D6C, 0x6007, 0x031C, 0xFF71, 0x1BD7, 0x6106, 0x03A4, 0xFF7E, 0x1A4C, 0x61F3, 0x0435, 0xFF8A, 0x18CB, 0x62CB, 0x04D1, 0xFF96, 0x1756, 0x638F, 0x0577, 0xFFA1, 0x15EB, 0x643F, 0x0628, 0xFFAC, 0x148C, 0x64D9, 0x06E4, 0xFFB6, 0x1338, 0x655E, 0x07AB, 0xFFBF, 0x11F0, 0x65CD, 0x087D, 0xFFC8, 0x10B4, 0x6626, 0x095A, 0xFFD0, 0x0F83, 0x6669, 0x0A44, 0xFFD8, 0x0E5F, 0x6696, 0x0B39, 0xFFDF, 0x0D46, 0x66AD, 0x0C39};

s32 MultAddLUT(s16* src, u32 srcPtr, u32 location)
{
    s16* lut = (s16*)((u8*)ResampleLUT + location);
    s32 accum = 0;
    for (int i = 0; i < 4; i++)
    {
        s32 temp = (s32) * (s16*)(src + MES(srcPtr + i)) * (s32)(s16)lut[i];
        accum += (s32)((temp + 0x4000) >> 15);
    }

    return accum;
}

void RESAMPLE()
{
    u8 Flags = (u8)(k0 >> 16 & 0xff);
    u32 Pitch = (k0 & 0xffff) << 1;
    u32 addy = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    u32 Accum = 0;
    u32 location;
    s16* dst;
    s16* src;
    dst = (s16*)BufferSpace;
    src = (s16*)BufferSpace;
    u32 srcPtr = AudioInBuffer / 2;
    u32 dstPtr = AudioOutBuffer / 2;

    /*
    if (addy > (1024*1024*8))
    addy = (t9 & 0xffffff);
    */
    srcPtr -= 4;

    if ((Flags & 0x1) == 0)
    {
        // memcpy (src+srcPtr, rdram+addy, 0x8);
        for (int x = 0; x < 4; x++)
            src[MES(srcPtr + x)] = ((u16*)DRAM)[MES(addy / 2 + x)];
        Accum = *(u16*)(DRAM + addy + 10);
    }
    else
    {
        for (int x = 0; x < 4; x++)
            src[MES(srcPtr + x)] = 0; //*(u16 *)(rdram + HES(addy + x));
    }

    assert((Flags & 0x2) == 0);

    for (int i = 0; i < (AudioCount + 0xf & 0xFFF0) / 2; i++)
    {
        // location = (((Accum * 0x40) >> 0x10) * 8);
        location = (Accum >> 0xa) << 0x3;

        // mov eax, dword ptr [src+srcPtr];
        // movsx edx, word ptr [lut];
        // shl edx, 1
        // imul edx
        // test eax, 08000h
        // setz ecx
        // shl ecx, 16
        // xor eax, 08000h
        // add eax, ecx
        // and edx, 0f000h

        // imul

        dst[MES(dstPtr)] = pack_signed(MultAddLUT(src, srcPtr, location));
        dstPtr++;
        Accum += Pitch;
        srcPtr += Accum >> 16;
        Accum &= 0xffff;
    }
    for (int x = 0; x < 4; x++)
        ((u16*)DRAM)[MES(addy / 2 + x)] = src[MES(srcPtr + x)];
    // memcpy (RSWORK, src+srcPtr, 0x8);
    *(u16*)(DRAM + addy + 10) = (u16)Accum;
}

void RESAMPLE2()
{
    u8 Flags = (u8)(k0 >> 16 & 0xff);
    u32 Pitch = (k0 & 0xffff) << 1;
    u32 addy = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    u32 Accum = 0;
    u32 location;
    s16* dst;
    s16* src;
    dst = (s16*)BufferSpace;
    src = (s16*)BufferSpace;
    u32 srcPtr = AudioInBuffer / 2;
    u32 dstPtr = AudioOutBuffer / 2;

    if (addy > 1024 * 1024 * 8)
        addy = t9 & 0xffffff;

    srcPtr -= 4;

    if ((Flags & 0x1) == 0)
    {
        for (int x = 0; x < 4; x++) // memcpy (src+srcPtr, rdram+addy, 0x8);
            src[MES(srcPtr + x)] = ((u16*)DRAM)[MES(addy / 2 + x)];
        Accum = *(u16*)(DRAM + addy + 10);
    }
    else
    {
        for (int x = 0; x < 4; x++)
            src[MES(srcPtr + x)] = 0; //*(u16 *)(rdram + HES(addy + x));
    }

    //	assert((Flags & 0x2) == 0);

    for (int i = 0; i < (AudioCount + 0xf & 0xFFF0) / 2; i++)
    {
        location = ((Accum * 0x40) >> 0x10) * 8;
        // location = (Accum >> 0xa) << 0x3;

        dst[MES(dstPtr)] = pack_signed(MultAddLUT(src, srcPtr, location));
        dstPtr++;
        Accum += Pitch;
        srcPtr += Accum >> 16;
        Accum &= 0xffff;
    }
    for (int x = 0; x < 4; x++)
        ((u16*)DRAM)[MES(addy / 2 + x)] = src[MES(srcPtr + x)];
    *(u16*)(DRAM + addy + 10) = (u16)Accum;
    // memcpy (RSWORK, src+srcPtr, 0x8);
}

void RESAMPLE3()
{
    u8 Flags = (u8)(t9 >> 0x1e);
    u32 Pitch = (t9 >> 0xe & 0xffff) << 1;
    u32 addy = k0 & 0xffffff;
    u32 Accum = 0;
    u32 location;
    s16* dst;
    s16* src;
    dst = (s16*)BufferSpace;
    src = (s16*)BufferSpace;
    u32 srcPtr = ((t9 >> 2 & 0xfff) + 0x4f0) / 2;
    u32 dstPtr; //=(AudioOutBuffer/2);

    // if (addy > (1024*1024*8))
    //	addy = (t9 & 0xffffff);

    srcPtr -= 4;

    if (t9 & 0x3)
    {
        dstPtr = 0x660 / 2;
    }
    else
    {
        dstPtr = 0x4f0 / 2;
    }

    if ((Flags & 0x1) == 0)
    {
        for (int x = 0; x < 4; x++) // memcpy (src+srcPtr, rdram+addy, 0x8);
            src[MES(srcPtr + x)] = ((u16*)DRAM)[MES(addy / 2 + x)];
        Accum = *(u16*)(DRAM + addy + 10);
    }
    else
    {
        for (int x = 0; x < 4; x++)
            src[MES(srcPtr + x)] = 0; //*(u16 *)(rdram + HES(addy + x));
    }

#ifdef _DEBUG
    assert((Flags & 0x2) == 0);
#endif

    for (int i = 0; i < 0x170 / 2; i++)
    {
        location = ((Accum * 0x40) >> 0x10) * 8;
        // location = (Accum >> 0xa) << 0x3;

        /*
        for (int i = 0; i < 4; i++)
        {
            temp =  ((s64)*(s16*)(src + MES(srcPtr+i))*((s64)((s16)lut[i]<<1)));
            if (temp & 0x8000) temp = (temp^0x8000) + 0x10000;
            else temp = (temp^0x8000);
            temp = (s32)(temp >> 16);
            accum = (s32)pack_signed(temp);
        }
        */

        dst[MES(dstPtr)] = pack_signed(MultAddLUT(src, srcPtr, location));
        dstPtr++;
        Accum += Pitch;
        srcPtr += Accum >> 16;
        Accum &= 0xffff;
    }
    for (int x = 0; x < 4; x++)
        ((u16*)DRAM)[MES(addy / 2 + x)] = src[MES(srcPtr + x)];
    *(u16*)(DRAM + addy + 10) = (u16)Accum;
}
