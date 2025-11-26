/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* memset() and memcpy() */
#include <string.h>

#include "audiohle.h"

u16 adpcmtable[0x88];

void InitInput(s16* inp, int index, u8 icode, u8 mask, u8 shifter, int vscale)
{
    inp[index] = (s16)((icode & mask) << shifter);
    inp[index] = (s16)((s32)(inp[index] * vscale) >> 16);
}

void ADPCM_madd(s32* a, s16* book1, s16* book2, s16 l1, s16 l2, s16* inp)
{
    __m128i xmm_source, xmm_target;
    __m128i prod_m, prod_n; /* [0] 0xMMMMNNNN, [1] 0xMMMMNNNN, ... [7] */
    __m128i prod_hi, prod_lo; /* (s32)[0, 1, 2, 3], (s32)[4, 5, 6, 7] */
    s32 accumulators[4];
    s16 b[8];
    register int i;

    xmm_source = _mm_set1_epi16(l1);
    xmm_target = _mm_loadu_si128((__m128i*)book1);
    prod_m = _mm_mulhi_epi16(xmm_target, xmm_source);
    prod_n = _mm_mullo_epi16(xmm_target, xmm_source);
    prod_hi = _mm_unpacklo_epi16(prod_n, prod_m);
    prod_lo = _mm_unpackhi_epi16(prod_n, prod_m);

    xmm_source = _mm_set1_epi16(l2);
    xmm_target = _mm_loadu_si128((__m128i*)book2);
    prod_m = _mm_mulhi_epi16(xmm_target, xmm_source);
    prod_n = _mm_mullo_epi16(xmm_target, xmm_source);
    xmm_target = _mm_unpacklo_epi16(prod_n, prod_m);
    xmm_source = _mm_unpackhi_epi16(prod_n, prod_m);

    /*
     *  for (i = 0; i < 8; i++)
     *      products[i]  = (l1[i] * book1[i]) + (l2[i] * book2[i]);
     */
    prod_hi = _mm_add_epi32(prod_hi, xmm_target);
    prod_lo = _mm_add_epi32(prod_lo, xmm_source);

    /*
     * for (i = 0; i < 8; i++)
     *     a[i] += inp[i] << 11;
     */
    xmm_source = _mm_loadu_si128((__m128i*)inp);
    prod_m = _mm_unpacklo_epi16(xmm_source, xmm_source); /* (xmm_source, any) */
    prod_n = _mm_unpackhi_epi16(xmm_source, xmm_source); /* Ignore upper 16b. */
    prod_m = _mm_slli_epi32(prod_m, 16); /* ready to sign-extend s16 to s32 */
    prod_n = _mm_slli_epi32(prod_n, 16);
    prod_m = _mm_srai_epi32(prod_m, 16 - 11); /* inp[i] << 11 = 2048 * inp[i] */
    prod_n = _mm_srai_epi32(prod_n, 16 - 11);

    prod_hi = _mm_add_epi32(prod_hi, prod_m);
    prod_lo = _mm_add_epi32(prod_lo, prod_n);
    _mm_storeu_si128((__m128i*)&a[0], prod_hi);
    _mm_storeu_si128((__m128i*)&a[4], prod_lo);

    _mm_storeu_si128((__m128i*)&b[0], _mm_setzero_si128());
    xmm_source = _mm_loadu_si128((__m128i*)inp);

    for (i = 0; i < 1; i++)
        b[i] = book2[0 - i];
    accumulators[0] = (s32)b[0] * (s32)inp[0];
    a[1] += accumulators[0];

    for (i = 0; i < 2; i++)
        b[i] = book2[1 - i];

    xmm_target = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    a[2] += accumulators[0];

    for (i = 0; i < 3; i++)
        b[i] = book2[2 - i];

    xmm_target = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    a[3] += accumulators[0] + accumulators[1];

    for (i = 0; i < 4; i++)
        b[i] = book2[3 - i];

    xmm_target = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    a[4] += accumulators[0] + accumulators[1];

    for (i = 0; i < 5; i++)
        b[i] = book2[4 - i];

    xmm_target = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    a[5] += accumulators[0] + accumulators[1] + accumulators[2];

    for (i = 0; i < 6; i++)
        b[i] = book2[5 - i];

    xmm_target = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    a[6] += accumulators[0] + accumulators[1] + accumulators[2];

    for (i = 0; i < 7; i++)
        b[i] = book2[6 - i];

    xmm_target = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    a[7] += accumulators[0] + accumulators[1] + accumulators[2] + accumulators[3];
}

void ADPCM()
{ // Work in progress! :)
    u8 Flags = (u8)(k0 >> 16 & 0xff);
    // u16 Gain = (u16)(k0 & 0xffff);
    u32 Address = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    u16 inPtr = 0;
    // s16 *out=(s16 *)(testbuff+(AudioOutBuffer>>2));
    s16* out = (s16*)(BufferSpace + AudioOutBuffer);
    // u8 *in = (u8 *)(BufferSpace + AudioInBuffer);
    s16 count = (s16)AudioCount;
    int vscale;
    u16 index;
    s32 a[8];
    s16 b[8];
    s16* book1;
    s16* book2;

    /*
    if (Address > (1024*1024*8))
    Address = (t9 & 0xffffff);
    */
    memset(out, 0, 32);

    if (!(Flags & 0x1))
    {
        if (Flags & 0x2)
        {
            memcpy(out, &DRAM[loopval], 32);
        }
        else
        {
            memcpy(out, &DRAM[Address], 32);
        }
    }

    s16 l1 = out[15];
    s16 l2 = out[14];
    s16 inp1[8];
    s16 inp2[8];
    out += 16;
    while (count > 0)
    {
        // the first interation through, these values are
        // either 0 in the case of A_INIT, from a special
        // area of memory in the case of A_LOOP or just
        // the values we calculated the last time

        u8 code = BufferSpace[BES(AudioInBuffer + inPtr)];
        index = code & 0xf;
        index <<= 4; // index into the adpcm code table
        book1 = (s16*)&adpcmtable[index];
        book2 = book1 + 8;
        code >>= 4; // upper nibble is scale
#if 0
		assert((12 - code) - 1 >= 0);
#endif
        vscale = 0x8000u >> (12 - code - 1); // very strange. 0x8000 would be .5 in 16:16 format
        // so this appears to be a fractional scale based
        // on the 12 based inverse of the scale value.  note
        // that this could be negative, in which case we do
        // not use the calculated vscale value...
        if (12 - code - 1 < 0)
            vscale = 0x10000; /* null operation:  << 16 then >> 16 */
        inPtr++; // coded adpcm data lies next
        for (int i = 0; i < 8; i += 2) // loop of 8, for 8 coded nibbles from 4 bytes
                                       // which yields 8 short pcm values
        {
            u8 icode = BufferSpace[BES(AudioInBuffer + inPtr)];
            inPtr++;

            InitInput(inp1, i + 0, icode, 0xF0, 8, vscale); // this will in effect be signed
            InitInput(inp1, i + 1, icode, 0x0F, 12, vscale);
        }
        for (int i = 0; i < 8; i += 2)
        {
            u8 icode = BufferSpace[BES(AudioInBuffer + inPtr)];
            inPtr++;

            InitInput(inp2, i + 0, icode, 0xF0, 8, vscale); // this will in effect be signed
            InitInput(inp2, i + 1, icode, 0x0F, 12, vscale);
        }

        ADPCM_madd(a, book1, book2, l1, l2, inp1);
        for (int i = 0; i < 8; i++)
            a[i] = a[i] >> 11;
        vsats128(&b[0], &a[0]);
        swap_elements(out, &b[0]);
        out += 8;

        l1 = b[6];
        l2 = b[7];

        ADPCM_madd(a, book1, book2, l1, l2, inp2);
        for (int i = 0; i < 8; i++)
            a[i] = a[i] >> 11;
        vsats128(&b[0], &a[0]);
        swap_elements(out, &b[0]);
        out += 8;

        l1 = b[6];
        l2 = b[7];

        count -= 32;
    }
    out -= 16;
    memcpy(&DRAM[Address], out, 32);
}

void ADPCM2()
{ // Verified to be 100% Accurate...
    u8 Flags = (u8)(k0 >> 16 & 0xff);
    //	u16 Gain = (u16)(k0 & 0xffff);
    u32 Address = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    u16 inPtr = 0;
    // s16 *out=(s16 *)(testbuff+(AudioOutBuffer>>2));
    s16* out = (s16*)(BufferSpace + AudioOutBuffer);
    //	u8 *in = (u8 *)(BufferSpace + AudioInBuffer);
    s16 count = (s16)AudioCount;
    int vscale;
    u16 index;
    s32 a[8];
    s16 b[8];
    s16* book1;
    s16* book2;

    u8 srange;
    // u8 inpinc;
    u8 mask1;
    u8 mask2;
    u8 shifter;

    memset(out, 0, 32);

    if (!(Flags & 0x1))
    {
        if (Flags & 0x2)
            memcpy(out, &DRAM[loopval], 32);
        else
            memcpy(out, &DRAM[Address], 32);
    }
    if (Flags & 0x4)
    { // Needed for Zelda MM
        srange = 0xE;
        // inpinc = 0x5;
        mask1 = 0xC0;
        mask2 = 0x30;
        shifter = 10;
    }
    else
    {
        srange = 0xC;
        // inpinc = 0x9;
        mask1 = 0xF0;
        mask2 = 0x0F;
        shifter = 12;
    }

    s16 l1 = out[15];
    s16 l2 = out[14];
    s16 inp1[8];
    s16 inp2[8];
    out += 16;
    while (count > 0)
    {
        u8 code = BufferSpace[BES(AudioInBuffer + inPtr)];
        index = code & 0xf;
        index <<= 4;
        book1 = (s16*)&adpcmtable[index];
        book2 = book1 + 8;
        code >>= 4;
#if 0
		assert((srange - code) - 1 >= 0);
#endif
        vscale = 0x8000u >> (srange - code - 1);
        if (srange - code - 1 < 0)
            vscale = 0x10000; /* null operation:  << 16 then >> 16 */
        inPtr++;

        for (int i = 0; i < 8;)
        {
            u8 icode = BufferSpace[BES(AudioInBuffer + inPtr)];
            inPtr++;

            InitInput(inp1, i + 0, icode, mask1, 8, vscale); // this will in effect be signed
            InitInput(inp1, i + 1, icode, mask2, shifter, vscale);
            i += 2;

            if (Flags & 4)
            {
                InitInput(inp1, i + 0, icode, 0xC, 12, vscale); // this will in effect be signed
                InitInput(inp1, i + 1, icode, 0x3, 14, vscale);
                i += 2;
            } // end flags
        } // end while

        for (int i = 0; i < 8;)
        {
            u8 icode = BufferSpace[BES(AudioInBuffer + inPtr)];
            inPtr++;

            InitInput(inp2, i + 0, icode, mask1, 8, vscale);
            InitInput(inp2, i + 1, icode, mask2, shifter, vscale);
            i += 2;

            if (Flags & 4)
            {
                InitInput(inp2, i + 0, icode, 0xC, 12, vscale);
                InitInput(inp2, i + 1, icode, 0x3, 14, vscale);
                i += 2;
            } // end flags
        }

        ADPCM_madd(a, book1, book2, l1, l2, inp1);
        for (int i = 0; i < 8; i++)
            a[i] = a[i] >> 11;
        vsats128(&b[0], &a[0]);
        swap_elements(out, &b[0]);
        out += 8;

        l1 = b[6];
        l2 = b[7];

        ADPCM_madd(a, book1, book2, l1, l2, inp2);
        for (int i = 0; i < 8; i++)
            a[i] = a[i] >> 11;
        vsats128(&b[0], &a[0]);
        swap_elements(out, &b[0]);
        out += 8;

        l1 = b[6];
        l2 = b[7];

        count -= 32;
    }
    out -= 16;
    memcpy(&DRAM[Address], out, 32);
}

void ADPCM3()
{ // Verified to be 100% Accurate...
    u8 Flags = (u8)(t9 >> 0x1c & 0xff);
    // u16 Gain=(u16)(k0&0xffff);
    u32 Address = k0 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    u16 inPtr = t9 >> 12 & 0xf;
    // s16 *out=(s16 *)(testbuff+(AudioOutBuffer>>2));
    s16* out = (s16*)(BufferSpace + (t9 & 0xfff) + 0x4f0);
    //	u8 *in = (u8 *)(BufferSpace + ((t9 >> 12) & 0xf) + 0x4f0);
    s16 count = (s16)(t9 >> 16 & 0xfff);
    int vscale;
    u16 index;
    s32 a[8];
    s16 b[8];
    s16* book1;
    s16* book2;

    memset(out, 0, 32);

    if (!(Flags & 0x1))
    {
        if (Flags & 0x2)
            memcpy(out, &DRAM[loopval], 32);
        else
            memcpy(out, &DRAM[Address], 32);
    }

    s16 l1 = out[15];
    s16 l2 = out[14];
    s16 inp1[8];
    s16 inp2[8];
    out += 16;
    while (count > 0)
    {
        // the first interation through, these values are
        // either 0 in the case of A_INIT, from a special
        // area of memory in the case of A_LOOP or just
        // the values we calculated the last time

        u8 code = BufferSpace[BES(0x4f0 + inPtr)];
        index = code & 0xf;
        index <<= 4; // index into the adpcm code table
        book1 = (s16*)&adpcmtable[index];
        book2 = book1 + 8;
        code >>= 4; // upper nibble is scale

        vscale = 0x8000u >> (12 - code - 1); // very strange. 0x8000 would be .5 in 16:16 format
        // so this appears to be a fractional scale based
        // on the 12 based inverse of the scale value.  note
        // that this could be negative, in which case we do
        // not use the calculated vscale value...
        if (12 - code - 1 < 0)
            vscale = 0x10000; /* null operation:  << 16 then >> 16 */

        inPtr++; // coded adpcm data lies next
        for (int i = 0; i < 8; i += 2) // loop of 8, for 8 coded nibbles from 4 bytes
                                       // which yields 8 short pcm values
        {
            u8 icode = BufferSpace[BES(0x4f0 + inPtr)];
            inPtr++;

            InitInput(inp1, i + 0, icode, 0xF0, 8, vscale); // this will in effect be signed
            InitInput(inp1, i + 1, icode, 0x0F, 12, vscale);
        }
        for (int i = 0; i < 8; i += 2)
        {
            u8 icode = BufferSpace[BES(0x4F0 + inPtr)];
            inPtr++;

            InitInput(inp2, i + 0, icode, 0xF0, 8, vscale); // this will in effect be signed
            InitInput(inp2, i + 1, icode, 0x0F, 12, vscale);
        }

        ADPCM_madd(a, book1, book2, l1, l2, inp1);
        for (int i = 0; i < 8; i++)
            a[i] = a[i] >> 11;
        vsats128(&b[0], &a[0]);
        swap_elements(out, &b[0]);
        out += 8;

        l1 = b[6];
        l2 = b[7];

        ADPCM_madd(a, book1, book2, l1, l2, inp2);
        for (int i = 0; i < 8; i++)
            a[i] = a[i] >> 11;
        vsats128(&b[0], &a[0]);
        swap_elements(out, &b[0]); // *(out + i + 0x1F8) = b[i ^ 1];
        out += 8;

        l1 = b[6];
        l2 = b[7];

        count -= 32;
    }
    out -= 16;
    memcpy(&DRAM[Address], out, 32);
}

void LOADADPCM()
{ // Loads an ADPCM table - Works 100% Now 03-13-01
    u32 v0;
    size_t i, limit;

    v0 = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    //	if (v0 > (1024*1024*8))
    //		v0 = (t9 & 0xffffff);
    //	memcpy (dmem+0x4c0, rdram+v0, k0&0xffff); // Could prolly get away with not putting this in dmem
    //	assert ((k0&0xffff) <= 0x80);
    u16* table = (u16*)(DRAM + v0);

    limit = (k0 & 0x0000FFFF) >> 4;
    for (i = 0; i < limit; i++)
        swap_elements(&adpcmtable[8 * i], &table[8 * i]);
}

void LOADADPCM2()
{ // Loads an ADPCM table - Works 100% Now 03-13-01
    u32 v0;
    size_t i, limit;

    v0 = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];
    u16* table = (u16*)(DRAM + v0); // Zelda2 Specific...

    limit = (k0 & 0x0000FFFF) >> 4;
    for (i = 0; i < limit; i++)
        swap_elements(&adpcmtable[8 * i], &table[8 * i]);
}

void LOADADPCM3()
{ // Loads an ADPCM table - Works 100% Now 03-13-01
    u32 v0;
    size_t i, limit;

    v0 = t9 & 0xffffff;
    // memcpy (dmem+0x3f0, rdram+v0, k0&0xffff);
    // assert ((k0&0xffff) <= 0x80);
    u16* table = (u16*)(DRAM + v0);

    limit = (k0 & 0x0000FFFF) >> 4;
    for (i = 0; i < limit; i++)
        swap_elements(&adpcmtable[8 * i], &table[8 * i]);
}
