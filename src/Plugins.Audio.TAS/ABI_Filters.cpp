/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* memset() and memcpy() */
#include <string.h>

#include "audiohle.h"

/*
 * Some combination of RSP LWC2 pack-type operations and vector multiply-
 * accumulate going on here, doing some fancy matrix math from data memory.
 */
static void packed_multiply_accumulate(i32* acc_in, i16* vs, i16* vt)
{
    i32 pre_buffer[8];
    i32 result;

    __m128i xmm_source, xmm_target;

    xmm_source = _mm_loadu_si128((__m128i*)vs);
    xmm_target = _mm_loadu_si128((__m128i*)vt);
    xmm_source = _mm_madd_epi16(xmm_source, xmm_target);
    _mm_storeu_si128((__m128i*)pre_buffer, xmm_source);
    result = pre_buffer[0] + pre_buffer[1] + pre_buffer[2] + pre_buffer[3];

    *acc_in = result;
}

// rdot is borrowed from Mupen64Plus audio.c file, modified for SSE2
s32 rdot_ABI(size_t n, const s16* x, const s16* y)
{
    __m128i xmm_source, xmm_target;
    s32 accumulators[4];
    s16 b[8]; /* (n <= 8) in all calls to this function. */
    register size_t i;

    y += n;
    --y;

    memset(&b[0], 0x0000, 8 * sizeof(s16));
    for (i = 0; i < n; i++)
        b[i] = *(y - i);

    xmm_target = _mm_loadu_si128((__m128i*)&x[0]);
    xmm_source = _mm_loadu_si128((__m128i*)&b[0]);
    xmm_target = _mm_madd_epi16(xmm_target, xmm_source);
    _mm_storeu_si128((__m128i*)&accumulators[0], xmm_target);

    return accumulators[0] + accumulators[1] +
    accumulators[2] + accumulators[3];
}

void FILTER2()
{
    int x, i;
    static int cnt = 0;
    static s16* lutt6;
    static s16* lutt5;
    u8* save = DRAM + (t9 & 0xFFFFFF);
    u8 t4 = (u8)(k0 >> 0x10 & 0xFF);

    if (t4 > 1)
    { // Then set the cnt variable
        cnt = k0 & 0xFFFF;
        lutt6 = (s16*)save;
        //				memcpy (dmem+0xFE0, rdram+(t9&0xFFFFFF), 0x10);
        return;
    }

    if (t4 == 0)
    {
        //				memcpy (dmem+0xFB0, rdram+(t9&0xFFFFFF), 0x20);
        lutt5 = (s16*)(save + 0x10);
    }

    lutt5 = (s16*)(save + 0x10);

    //			lutt5 = (s16 *)(dmem + 0xFC0);
    //			lutt6 = (s16 *)(dmem + 0xFE0);
    for (x = 0; x < 8; x++)
    {
        s32 a;
        a = (lutt5[x] + lutt6[x]) >> 1;
        lutt5[x] = lutt6[x] = (s16)a;
    }
    i16 inputs_matrix[16];
    i16* inp1;
    i16* inp2;
    s32 out1[8];
    s16 outbuff[0x3c0], *outp;
    u32 inPtr = (u32)(k0 & 0xffff);

    inp1 = (i16*)save;
    outp = outbuff;
    inp2 = (i16*)(BufferSpace + inPtr);

    /*
     * The first iteration has no contiguity between inp1 and inp2.
     * Every iteration thereafter, they are contiguous:  inp1 = inp2; inp2 += 8;
     */
    for (i = 0; i < 8; i++)
        inputs_matrix[15 - (i + 0)] = inp1[i];
    swap_elements(&inputs_matrix[8], &inputs_matrix[8]);
    for (i = 0; i < 8; i++)
        inputs_matrix[15 - (i + 8)] = inp2[i];
    swap_elements(&inputs_matrix[0], &inputs_matrix[0]);

    for (x = 0; x < cnt; x += 0x10)
    {
        packed_multiply_accumulate(&out1[0], &inputs_matrix[6], &lutt6[0]);
        packed_multiply_accumulate(&out1[1], &inputs_matrix[7], &lutt6[0]);
        packed_multiply_accumulate(&out1[2], &inputs_matrix[4], &lutt6[0]);
        packed_multiply_accumulate(&out1[3], &inputs_matrix[5], &lutt6[0]);
        packed_multiply_accumulate(&out1[4], &inputs_matrix[2], &lutt6[0]);
        packed_multiply_accumulate(&out1[5], &inputs_matrix[3], &lutt6[0]);
        packed_multiply_accumulate(&out1[6], &inputs_matrix[0], &lutt6[0]);
        packed_multiply_accumulate(&out1[7], &inputs_matrix[1], &lutt6[0]);

        for (i = 0; i < 8; i++)
            out1[i] += 0x4000;
        for (i = 0; i < 8; i++)
            out1[i] >>= 15;
        vsats128(&outp[0], &out1[0]);
        outp += 8;

        inp1 = inp2 + 0;
        inp2 = inp2 + 8;

        for (i = 0; i < 16; i++)
            inputs_matrix[15 - i] = inp1[i];
        swap_elements(&inputs_matrix[0], &inputs_matrix[0]);
        swap_elements(&inputs_matrix[8], &inputs_matrix[8]);
    }
    //			memcpy (rdram+(t9&0xFFFFFF), dmem+0xFB0, 0x20);
    memcpy(save, inp2 - 8, 0x10);
    memcpy(BufferSpace + (k0 & 0xffff), outbuff, cnt);
}

extern u16 adpcmtable[]; // size of 0x88 * 2

// POLEF filter - Much of the code is borrowed from Mupen64Plus
// We need to refactor for enabling POLEF across the board as well
void POLEF()
{
    __m128i xmm_source, xmm_target, prod_hi, prod_lo, prod_m, prod_n, xmm_gain;
    u8 Flags = (u8)(k0 >> 16 & 0xff);
    s16 Gain = (u16)(k0 & 0xffff);
    u32 Address = t9 & 0xffffff; // + SEGMENTS[(t9>>24)&0xf];

    s16* dst = (s16*)(BufferSpace + AudioOutBuffer);

    const s16* const h1 = (s16*)adpcmtable;
    s16* const h2 = (s16*)adpcmtable + 8;

    unsigned int i;
    s16 l1, l2;
    s16 h2_before[8];
    if (AudioCount == 0)
        return;
    int count = AudioCount + 15 & ~15;

    if (Flags & A_INIT)
    {
        l1 = 0;
        l2 = 0;
    }
    else
    {
        memcpy(hleMixerWorkArea, DRAM + Address, 8);
        l1 = hleMixerWorkArea[2];
        l2 = hleMixerWorkArea[3];
    }

    xmm_target = _mm_loadu_si128((__m128i*)h2);
    xmm_gain = _mm_set1_epi16(Gain);
    _mm_storeu_si128((__m128i*)&h2_before[0], xmm_target);

    prod_m = _mm_mulhi_epi16(xmm_target, xmm_gain);
    prod_n = _mm_mullo_epi16(xmm_target, xmm_gain);
    prod_hi = _mm_unpacklo_epi16(prod_n, prod_m);
    prod_lo = _mm_unpackhi_epi16(prod_n, prod_m);
    prod_hi = _mm_srai_epi32(prod_hi, 14);
    prod_lo = _mm_srai_epi32(prod_lo, 14);
    prod_hi = _mm_packs_epi32(prod_hi, prod_lo);
    _mm_storeu_si128((__m128i*)&h2[0], prod_hi);

    s16* inp = (s16*)(BufferSpace + AudioInBuffer);

    do
    {
        s32 accumulators[8];
        s16 frame[8];

        swap_elements(&frame[0], &inp[0]);
        xmm_target = _mm_loadu_si128((__m128i*)&frame[0]);

        prod_m = _mm_mulhi_epi16(xmm_target, xmm_gain);
        prod_n = _mm_mullo_epi16(xmm_target, xmm_gain);
        prod_hi = _mm_unpacklo_epi16(prod_n, prod_m);
        prod_lo = _mm_unpackhi_epi16(prod_n, prod_m);

        xmm_source = _mm_set1_epi16(l1);
        xmm_target = _mm_loadu_si128((__m128i*)&h1[0]);
        prod_m = _mm_mulhi_epi16(xmm_target, xmm_source);
        prod_n = _mm_mullo_epi16(xmm_target, xmm_source);
        xmm_source = _mm_unpacklo_epi16(prod_n, prod_m);
        xmm_target = _mm_unpackhi_epi16(prod_n, prod_m);
        prod_hi = _mm_add_epi32(prod_hi, xmm_source);
        prod_lo = _mm_add_epi32(prod_lo, xmm_target);

        xmm_source = _mm_set1_epi16(l2);
        xmm_target = _mm_loadu_si128((__m128i*)&h2_before[0]);
        prod_m = _mm_mulhi_epi16(xmm_target, xmm_source);
        prod_n = _mm_mullo_epi16(xmm_target, xmm_source);
        xmm_source = _mm_unpacklo_epi16(prod_n, prod_m);
        xmm_target = _mm_unpackhi_epi16(prod_n, prod_m);
        prod_hi = _mm_add_epi32(prod_hi, xmm_source);
        prod_lo = _mm_add_epi32(prod_lo, xmm_target);

        _mm_storeu_si128((__m128i*)&accumulators[0], prod_hi);
        _mm_storeu_si128((__m128i*)&accumulators[4], prod_lo);

        for (i = 0; i < 8; i++)
            accumulators[i] += rdot_ABI(i, &h2[0], &frame[0]);
        for (i = 0; i < 8; i++)
            accumulators[i] >>= 14;
        vsats128(&dst[0], &accumulators[0]);

        swap_elements(&dst[0], &dst[0]);
        l1 = dst[MES(6)];
        l2 = dst[MES(7)];

        dst += 8;
        inp += 8;
        count -= 16;
    }
    while (count != 0);

    hleMixerWorkArea[2] = l1;
    hleMixerWorkArea[3] = l2;
    memcpy(DRAM + Address, (u8*)hleMixerWorkArea, 8);
}
