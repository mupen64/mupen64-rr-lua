/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "HLE.hpp"
#include <tmmintrin.h>

/******** DMEM Memory Map for ABI 1 ***************
Address/Range		Description
-------------		-------------------------------
0x000..0x2BF		UCodeData
    0x000-0x00F		Constants  - 0000 0001 0002 FFFF 0020 0800 7FFF 4000
    0x010-0x02F		Function Jump Table (16 Functions * 2 bytes each = 32) 0x20
    0x030-0x03F		Constants  - F000 0F00 00F0 000F 0001 0010 0100 1000
    0x040-0x03F		Used by the Envelope Mixer (But what for?)
    0x070-0x07F		Used by the Envelope Mixer (But what for?)
0x2C0..0x31F		<Unknown>
0x320..0x35F		Segments
0x360				Audio In Buffer (Location)
0x362				Audio Out Buffer (Location)
0x364				Audio Buffer Size (Location)
0x366				Initial Volume for Left Channel
0x368				Initial Volume for Right Channel
0x36A				Auxillary Buffer #1 (Location)
0x36C				Auxillary Buffer #2 (Location)
0x36E				Auxillary Buffer #3 (Location)
0x370				Loop Value (shared location)
0x370				Target Volume (Left)
0x372				Ramp?? (Left)
0x374				Rate?? (Left)
0x376				Target Volume (Right)
0x378				Ramp?? (Right)
0x37A				Rate?? (Right)
0x37C				Dry??
0x37E				Wet??
0x380..0x4BF		Alist data
0x4C0..0x4FF		ADPCM CodeBook
0x500..0x5BF		<Unknown>
0x5C0..0xF7F		Buffers...
0xF80..0xFFF		<Unknown>
***************************************************/

static void SPNOOP()
{
}

uint32_t SEGMENTS[0x10]; // 0x0320
uint16_t AudioInBuffer;  // 0x0000(T8)
uint16_t AudioOutBuffer; // 0x0002(T8)
uint16_t AudioCount;     // 0x0004(T8)
int16_t Vol_Left;        // 0x0006(T8)
int16_t Vol_Right;       // 0x0008(T8)
uint16_t AudioAuxA;      // 0x000A(T8)
uint16_t AudioAuxC;      // 0x000C(T8)
uint16_t AudioAuxE;      // 0x000E(T8)
uint32_t loopval;        // 0x0010(T8) // Value set by A_SETLOOP : Possible conflict with SETVOLUME???
int16_t VolTrg_Left;     // 0x0010(T8)
int32_t VolRamp_Left;    // m_LeftVolTarget
int16_t VolTrg_Right;  // m_RightVol
int32_t VolRamp_Right; // m_RightVolTarget
int16_t Env_Dry; // 0x001C(T8)
int16_t Env_Wet; // 0x001E(T8)

uint8_t BufferSpace[0x10000];

int16_t hleMixerWorkArea[256];
uint16_t adpcmtable[0x88];

uint16_t ResampleLUT[0x200] = {
    0x0C39, 0x66AD, 0x0D46, 0xFFDF, 0x0B39, 0x6696, 0x0E5F, 0xFFD8, 0x0A44, 0x6669, 0x0F83, 0xFFD0, 0x095A, 0x6626,
    0x10B4, 0xFFC8, 0x087D, 0x65CD, 0x11F0, 0xFFBF, 0x07AB, 0x655E, 0x1338, 0xFFB6, 0x06E4, 0x64D9, 0x148C, 0xFFAC,
    0x0628, 0x643F, 0x15EB, 0xFFA1, 0x0577, 0x638F, 0x1756, 0xFF96, 0x04D1, 0x62CB, 0x18CB, 0xFF8A, 0x0435, 0x61F3,
    0x1A4C, 0xFF7E, 0x03A4, 0x6106, 0x1BD7, 0xFF71, 0x031C, 0x6007, 0x1D6C, 0xFF64, 0x029F, 0x5EF5, 0x1F0B, 0xFF56,
    0x022A, 0x5DD0, 0x20B3, 0xFF48, 0x01BE, 0x5C9A, 0x2264, 0xFF3A, 0x015B, 0x5B53, 0x241E, 0xFF2C, 0x0101, 0x59FC,
    0x25E0, 0xFF1E, 0x00AE, 0x5896, 0x27A9, 0xFF10, 0x0063, 0x5720, 0x297A, 0xFF02, 0x001F, 0x559D, 0x2B50, 0xFEF4,
    0xFFE2, 0x540D, 0x2D2C, 0xFEE8, 0xFFAC, 0x5270, 0x2F0D, 0xFEDB, 0xFF7C, 0x50C7, 0x30F3, 0xFED0, 0xFF53, 0x4F14,
    0x32DC, 0xFEC6, 0xFF2E, 0x4D57, 0x34C8, 0xFEBD, 0xFF0F, 0x4B91, 0x36B6, 0xFEB6, 0xFEF5, 0x49C2, 0x38A5, 0xFEB0,
    0xFEDF, 0x47ED, 0x3A95, 0xFEAC, 0xFECE, 0x4611, 0x3C85, 0xFEAB, 0xFEC0, 0x4430, 0x3E74, 0xFEAC, 0xFEB6, 0x424A,
    0x4060, 0xFEAF, 0xFEAF, 0x4060, 0x424A, 0xFEB6, 0xFEAC, 0x3E74, 0x4430, 0xFEC0, 0xFEAB, 0x3C85, 0x4611, 0xFECE,
    0xFEAC, 0x3A95, 0x47ED, 0xFEDF, 0xFEB0, 0x38A5, 0x49C2, 0xFEF5, 0xFEB6, 0x36B6, 0x4B91, 0xFF0F, 0xFEBD, 0x34C8,
    0x4D57, 0xFF2E, 0xFEC6, 0x32DC, 0x4F14, 0xFF53, 0xFED0, 0x30F3, 0x50C7, 0xFF7C, 0xFEDB, 0x2F0D, 0x5270, 0xFFAC,
    0xFEE8, 0x2D2C, 0x540D, 0xFFE2, 0xFEF4, 0x2B50, 0x559D, 0x001F, 0xFF02, 0x297A, 0x5720, 0x0063, 0xFF10, 0x27A9,
    0x5896, 0x00AE, 0xFF1E, 0x25E0, 0x59FC, 0x0101, 0xFF2C, 0x241E, 0x5B53, 0x015B, 0xFF3A, 0x2264, 0x5C9A, 0x01BE,
    0xFF48, 0x20B3, 0x5DD0, 0x022A, 0xFF56, 0x1F0B, 0x5EF5, 0x029F, 0xFF64, 0x1D6C, 0x6007, 0x031C, 0xFF71, 0x1BD7,
    0x6106, 0x03A4, 0xFF7E, 0x1A4C, 0x61F3, 0x0435, 0xFF8A, 0x18CB, 0x62CB, 0x04D1, 0xFF96, 0x1756, 0x638F, 0x0577,
    0xFFA1, 0x15EB, 0x643F, 0x0628, 0xFFAC, 0x148C, 0x64D9, 0x06E4, 0xFFB6, 0x1338, 0x655E, 0x07AB, 0xFFBF, 0x11F0,
    0x65CD, 0x087D, 0xFFC8, 0x10B4, 0x6626, 0x095A, 0xFFD0, 0x0F83, 0x6669, 0x0A44, 0xFFD8, 0x0E5F, 0x6696, 0x0B39,
    0xFFDF, 0x0D46, 0x66AD, 0x0C39};

static void CLEARBUFF()
{
    uint32_t addr = (uint32_t)(inst1 & 0xffff);
    uint32_t count = (uint32_t)(inst2 & 0xffff);
    addr &= 0xFFFC;
    memset(BufferSpace + addr, 0, (count + 3) & 0xFFFC);
}

static void ENVMIXER()
{
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    uint32_t addy = (inst2 & 0xFFFFFF);
    int16_t *inp = (int16_t *)(BufferSpace + AudioInBuffer);
    int16_t *out = (int16_t *)(BufferSpace + AudioOutBuffer);
    int16_t *aux1 = (int16_t *)(BufferSpace + AudioAuxA);
    int16_t *aux2 = (int16_t *)(BufferSpace + AudioAuxC);
    int16_t *aux3 = (int16_t *)(BufferSpace + AudioAuxE);

    uint16_t AuxIncRate = 1;
    int16_t zero[8];
    memset(zero, 0, 16);

    int32_t LTrg, RTrg;
    int16_t Wet, Dry;
    int32_t LRamp, RRamp;
    int32_t LAdderStart, RAdderStart, LAdderEnd, RAdderEnd;

    if (flags & A_INIT)
    {
        Wet = (int16_t)Env_Wet;
        Dry = (int16_t)Env_Dry;
        LTrg = VolTrg_Left << 16;
        RTrg = VolTrg_Right << 16;
        LAdderStart = Vol_Left << 16;
        RAdderStart = Vol_Right << 16;
        LAdderEnd = Vol_Left * (int32_t)VolRamp_Left;
        RAdderEnd = Vol_Right * (int32_t)VolRamp_Right;
        LRamp = VolRamp_Left;
        RRamp = VolRamp_Right;
    }
    else
    {
        memcpy((uint8_t *)hleMixerWorkArea, rsp.rdram + addy, 80);
        Wet = *(int16_t *)(hleMixerWorkArea + 0);
        Dry = *(int16_t *)(hleMixerWorkArea + 2);
        LTrg = *(int32_t *)(hleMixerWorkArea + 4);
        RTrg = *(int32_t *)(hleMixerWorkArea + 6);
        LRamp = *(int32_t *)(hleMixerWorkArea + 8);
        RRamp = *(int32_t *)(hleMixerWorkArea + 10);
        LAdderEnd = *(int32_t *)(hleMixerWorkArea + 12);
        RAdderEnd = *(int32_t *)(hleMixerWorkArea + 14);
        LAdderStart = *(int32_t *)(hleMixerWorkArea + 16);
        RAdderStart = *(int32_t *)(hleMixerWorkArea + 18);
    }

    if (!(flags & A_AUX))
    {
        AuxIncRate = 0;
        aux2 = aux3 = zero;
    }

    // Shuffle control: swaps adjacent int16 word pairs within each 64-bit lane,
    // i.e. models the [ptr ^ 1] index pattern used throughout the original.
    enum
    {
        SW = _MM_SHUFFLE(2, 3, 0, 1)
    };

    // Hoist invariant broadcast vectors outside the loop.
    const __m128i vDry = _mm_set1_epi16(Dry);
    const __m128i vWet = _mm_set1_epi16(Wet);

    uint32_t ptr = 0;

    for (int32_t y = 0; y < AudioCount; y += 0x10)
    {
        // ------------------------------------------------------------
        // Phase 1 – scalar: advance both envelope ramps and record the
        // upper 16 bits (the "volume") of each of the 8 per-sample
        // accumulators, honouring the linear step and target clamp.
        // ------------------------------------------------------------
        int16_t lvArr[8], rvArr[8];

        { // L channel
            int32_t acc, vol;
            if (LAdderStart != LTrg)
            {
                acc = LAdderStart;
                vol = (LAdderEnd - acc) >> 3;
                LAdderEnd = (int32_t)(((int64_t)LAdderEnd * LRamp) >> 16);
                LAdderStart = (int32_t)(((int64_t)acc * LRamp) >> 16);
            }
            else
            {
                acc = LTrg;
                vol = 0;
            }
            for (int x = 0; x < 8; x++)
            {
                acc += vol;
                if (vol <= 0)
                {
                    if (acc < LTrg)
                    {
                        acc = LTrg;
                        LAdderStart = LTrg;
                    }
                }
                else
                {
                    if (acc > LTrg)
                    {
                        acc = LTrg;
                        LAdderStart = LTrg;
                    }
                }
                lvArr[x] = (int16_t)(acc >> 16);
            }
        }

        { // R channel
            int32_t acc, vol;
            if (RAdderStart != RTrg)
            {
                acc = RAdderStart;
                vol = (RAdderEnd - acc) >> 3;
                RAdderEnd = (int32_t)(((int64_t)RAdderEnd * RRamp) >> 16);
                RAdderStart = (int32_t)(((int64_t)acc * RRamp) >> 16);
            }
            else
            {
                acc = RTrg;
                vol = 0;
            }
            for (int x = 0; x < 8; x++)
            {
                acc += vol;
                if (vol <= 0)
                {
                    if (acc < RTrg)
                    {
                        acc = RTrg;
                        RAdderStart = RTrg;
                    }
                }
                else
                {
                    if (acc > RTrg)
                    {
                        acc = RTrg;
                        RAdderStart = RTrg;
                    }
                }
                rvArr[x] = (int16_t)(acc >> 16);
            }
        }

        // ------------------------------------------------------------
        // Phase 2 – SIMD: build per-sample volume coefficients and mix
        // into each output buffer.
        //
        // The original accesses every buffer at [ptr ^ 1], which swaps
        // adjacent int16 pairs.  Rather than shuffling every audio load
        // and store, we swap the volume vectors once:
        //
        //   vmainR[y] = (Dry * rv[y^1] + 0x4000) >> 15
        //
        // so that a plain element-wise multiply with the unshuffled inp
        // and a plain saturating add to the unshuffled out reproduce the
        // original result exactly.
        //
        // _mm_mulhrs_epi16(a, b) == (a * b + 0x4000) >> 15  ← exact match
        // _mm_adds_epi16         == saturating int16 add     ← exact match
        // ------------------------------------------------------------

        // Load and swap-adjacent the volume arrays (models [ptr ^ 1] access).
        __m128i vlv = _mm_loadu_si128((const __m128i *)lvArr);
        vlv = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vlv, SW), SW);

        __m128i vrv = _mm_loadu_si128((const __m128i *)rvArr);
        vrv = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vrv, SW), SW);

        // Volume coefficients for the dry (main) path.
        __m128i vmainL = _mm_mulhrs_epi16(vDry, vlv); // → aux1
        __m128i vmainR = _mm_mulhrs_epi16(vDry, vrv); // → out

        __m128i vinp = _mm_loadu_si128((const __m128i *)(inp + ptr));

        // out[y] = saturate16(out[y] + (inp[y] * mainR[y] + 0x4000) >> 15)
        {
            __m128i v = _mm_loadu_si128((const __m128i *)(out + ptr));
            _mm_storeu_si128((__m128i *)(out + ptr), _mm_adds_epi16(v, _mm_mulhrs_epi16(vinp, vmainR)));
        }

        // aux1[y] = saturate16(aux1[y] + (inp[y] * mainL[y] + 0x4000) >> 15)
        {
            __m128i v = _mm_loadu_si128((const __m128i *)(aux1 + ptr));
            _mm_storeu_si128((__m128i *)(aux1 + ptr), _mm_adds_epi16(v, _mm_mulhrs_epi16(vinp, vmainL)));
        }

        if (AuxIncRate)
        {
            __m128i vauxL = _mm_mulhrs_epi16(vWet, vlv); // → aux3
            __m128i vauxR = _mm_mulhrs_epi16(vWet, vrv); // → aux2

            {
                __m128i v = _mm_loadu_si128((const __m128i *)(aux2 + ptr));
                _mm_storeu_si128((__m128i *)(aux2 + ptr), _mm_adds_epi16(v, _mm_mulhrs_epi16(vinp, vauxR)));
            }
            {
                __m128i v = _mm_loadu_si128((const __m128i *)(aux3 + ptr));
                _mm_storeu_si128((__m128i *)(aux3 + ptr), _mm_adds_epi16(v, _mm_mulhrs_epi16(vinp, vauxL)));
            }
        }

        ptr += 8;
    }

    *(int16_t *)(hleMixerWorkArea + 0) = Wet;
    *(int16_t *)(hleMixerWorkArea + 2) = Dry;
    *(int32_t *)(hleMixerWorkArea + 4) = LTrg;
    *(int32_t *)(hleMixerWorkArea + 6) = RTrg;
    *(int32_t *)(hleMixerWorkArea + 8) = LRamp;
    *(int32_t *)(hleMixerWorkArea + 10) = RRamp;
    *(int32_t *)(hleMixerWorkArea + 12) = LAdderEnd;
    *(int32_t *)(hleMixerWorkArea + 14) = RAdderEnd;
    *(int32_t *)(hleMixerWorkArea + 16) = LAdderStart;
    *(int32_t *)(hleMixerWorkArea + 18) = RAdderStart;
    memcpy(rsp.rdram + addy, (uint8_t *)hleMixerWorkArea, 80);
}

static void ENVMIXERo()
{
    // Borrowed from RCP...
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    uint32_t addy = (inst2 & 0xffffff); // + SEGMENTS[(inst2>>24)&0xf];

    int16_t *inp = (int16_t *)(BufferSpace + AudioInBuffer);
    int16_t *out = (int16_t *)(BufferSpace + AudioOutBuffer);
    int16_t *aux1 = (int16_t *)(BufferSpace + AudioAuxA);
    int16_t *aux2 = (int16_t *)(BufferSpace + AudioAuxC);
    int16_t *aux3 = (int16_t *)(BufferSpace + AudioAuxE);

    int32_t i1, o1, a1, a2, a3;
    int32_t MainR;
    int32_t MainL;
    int32_t AuxR;
    int32_t AuxL;

    uint16_t AuxIncRate = 1;
    int16_t zero[8];
    memset(zero, 0, 16);
    if (flags & A_INIT)
    {
        MainR = (Env_Dry * VolTrg_Right + 0x10000) >> 15;
        MainL = (Env_Dry * VolTrg_Left + 0x10000) >> 15;
        AuxR = (Env_Wet * VolTrg_Right + 0x8000) >> 16;
        AuxL = (Env_Wet * VolTrg_Left + 0x8000) >> 16;
    }
    else
    {
        memcpy((uint8_t *)hleMixerWorkArea, (rsp.rdram + addy), 80);
        MainR = hleMixerWorkArea[0];
        MainL = hleMixerWorkArea[2];
        AuxR = hleMixerWorkArea[4];
        AuxL = hleMixerWorkArea[6];
    }
    if (!(flags & A_AUX))
    {
        AuxIncRate = 0;
        aux2 = aux3 = zero;
    }
    for (int32_t i = 0; i < AudioCount / 2; i++)
    {
        i1 = (int32_t)(*inp);
        inp++;
        o1 = (int32_t)*out;
        a1 = (int32_t)*aux1;
        a2 = (int32_t)*aux2;
        a3 = (int32_t)*aux3;

        o1 = ((o1 * 0x7fff) + (i1 * MainR) + 0x10000) >> 15;
        a2 = ((a2 * 0x7fff) + (i1 * AuxR) + 0x8000) >> 16;

        a1 = ((a1 * 0x7fff) + (i1 * MainL) + 0x10000) >> 15;
        a3 = ((a3 * 0x7fff) + (i1 * AuxL) + 0x8000) >> 16;

        if (o1 > 32767)
            o1 = 32767;
        else if (o1 < -32768)
            o1 = -32768;

        if (a1 > 32767)
            a1 = 32767;
        else if (a1 < -32768)
            a1 = -32768;

        if (a2 > 32767)
            a2 = 32767;
        else if (a2 < -32768)
            a2 = -32768;

        if (a3 > 32767)
            a3 = 32767;
        else if (a3 < -32768)
            a3 = -32768;

        *(out++) = o1;
        *(aux1++) = a1;
        *aux2 = a2;
        *aux3 = a3;
        aux2 += AuxIncRate;
        aux3 += AuxIncRate;
    }
    hleMixerWorkArea[0] = MainR;
    hleMixerWorkArea[2] = MainL;
    hleMixerWorkArea[4] = AuxR;
    hleMixerWorkArea[6] = AuxL;
    memcpy(rsp.rdram + addy, (uint8_t *)hleMixerWorkArea, 80);
}

static void RESAMPLE()
{
    const uint8_t Flags = (uint8_t)((inst1 >> 16) & 0xff);
    const uint32_t Pitch = (inst1 & 0xffff) << 1;
    const uint32_t addy = inst2 & 0xffffff;
    uint32_t Accum = 0;

    int16_t *const dst = (int16_t *)BufferSpace;
    int16_t *const src = (int16_t *)BufferSpace;
    uint32_t srcPtr = (AudioInBuffer / 2) - 4;
    uint32_t dstPtr = AudioOutBuffer / 2;

    if (!(Flags & 0x1))
    {
        const uint16_t *rdram16 = (const uint16_t *)rsp.rdram;
        const uint32_t addy16 = addy / 2;
        for (int x = 0; x < 4; x++) src[(srcPtr + x) ^ 1] = (int16_t)rdram16[(addy16 + x) ^ 1];
        Accum = *(const uint16_t *)(rsp.rdram + addy + 10);
    }
    else
    {
        for (int x = 0; x < 4; x++) src[(srcPtr + x) ^ 1] = 0;
    }

    const int output_samples = ((AudioCount + 0xf) & 0xFFF0) / 2;

    const __m128i shuf_even = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 5, 4, 7, 6, 1, 0, 3, 2);
    const __m128i shuf_odd = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 11, 10, 5, 4, 7, 6, 1, 0);

    for (int i = 0; i < output_samples; i++)
    {
        const int16_t *coeff = (const int16_t *)((const uint8_t *)ResampleLUT + ((Accum >> 10) << 3));

        __m128i vsamples;

        if (srcPtr & 1)
        {
            const int16_t *base = src + (srcPtr - 1);
            __m128i combined = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)(base)),
                                                  _mm_loadl_epi64((const __m128i *)(base + 4)));
            vsamples = _mm_shuffle_epi8(combined, shuf_odd);
        }
        else
        {
            // Even: all 4 needed samples are contiguous, just swap pairs
            vsamples = _mm_shuffle_epi8(_mm_loadl_epi64((const __m128i *)(src + srcPtr)), shuf_even);
        }

        const __m128i vcoeffs = _mm_loadl_epi64((const __m128i *)coeff);
        const __m128i vprod = _mm_madd_epi16(vsamples, vcoeffs);

        int32_t sum = _mm_cvtsi128_si32(_mm_add_epi32(vprod, _mm_srli_si128(vprod, 4)));
        sum >>= 15;

        sum = sum > 32767 ? 32767 : (sum < -32768 ? -32768 : sum);
        dst[dstPtr ^ 1] = (int16_t)sum;
        dstPtr++;

        Accum += Pitch;
        srcPtr += (Accum >> 16);
        Accum &= 0xFFFF;
    }

    const uint32_t addy16 = addy / 2;
    for (int x = 0; x < 4; x++) ((uint16_t *)rsp.rdram)[(addy16 + x) ^ 1] = (uint16_t)src[(srcPtr + x) ^ 1];
    *(uint16_t *)(rsp.rdram + addy + 10) = (uint16_t)Accum;
}

static void SETVOL()
{
    // Might be better to unpack these depending on the flags...
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    uint16_t vol = (int16_t)(inst1 & 0xffff);
    uint16_t voltarg = (uint16_t)((inst2 >> 16) & 0xffff);
    uint16_t volrate = (uint16_t)((inst2 & 0xffff));

    if (flags & A_AUX)
    {
        Env_Dry = (int16_t)vol;     // m_MainVol
        Env_Wet = (int16_t)volrate; // m_AuxVol
        return;
    }

    if (flags & A_VOL)
    {
        // Set the Source(start) Volumes
        if (flags & A_LEFT)
        {
            Vol_Left = (int16_t)vol; // m_LeftVolume
        }
        else
        {
            // A_RIGHT
            Vol_Right = (int16_t)vol; // m_RightVolume
        }
        return;
    }

    // 0x370				Loop Value (shared location)
    // 0x370				Target Volume (Left)
    // uint16_t VolRamp_Left;	// 0x0012(T8)
    if (flags & A_LEFT)
    {
        // Set the Ramping values Target, Ramp
        VolTrg_Left = *(int16_t *)&inst1;
        VolRamp_Left = *(int32_t *)&inst2;
    }
    else
    {
        // A_RIGHT
        VolTrg_Right = *(int16_t *)&inst1;
        VolRamp_Right = *(int32_t *)&inst2;
    }
}

static void UNKNOWN()
{
}

static void SETLOOP()
{
    loopval = (inst2 & 0xffffff); // + SEGMENTS[(inst2>>24)&0xf];
}

static void ADPCM()
{
    // Work in progress! :)
    uint8_t Flags = (uint8_t)(inst1 >> 16) & 0xff;
    uint16_t Gain = (uint16_t)(inst1 & 0xffff);
    uint32_t Address = (inst2 & 0xffffff); // + SEGMENTS[(inst2>>24)&0xf];
    uint16_t inPtr = 0;
    int16_t *out = (int16_t *)(BufferSpace + AudioOutBuffer);
    uint8_t *in = (uint8_t *)(BufferSpace + AudioInBuffer);
    int16_t count = (int16_t)AudioCount;
    uint8_t icode;
    uint8_t code;
    int32_t vscale;
    uint16_t index;
    uint16_t j;
    int32_t a[8];
    int16_t *book1;
    int16_t *book2;
    memset(out, 0, 32);

    if (!(Flags & 0x1))
    {
        if (Flags & 0x2)
        {
            memcpy(out, &rsp.rdram[loopval & 0x7fffff], 32);
        }
        else
        {
            memcpy(out, &rsp.rdram[Address], 32);
        }
    }

    int32_t l1 = out[15];
    int32_t l2 = out[14];
    int32_t inp1[8];
    int32_t inp2[8];
    out += 16;
    while (count > 0)
    {
        // the first interation through, these values are
        // either 0 in the case of A_INIT, from a special
        // area of memory in the case of A_LOOP or just
        // the values we calculated the last time

        code = BufferSpace[(AudioInBuffer + inPtr) ^ 3];
        index = code & 0xf;
        index <<= 4; // index into the adpcm code table
        book1 = (int16_t *)&adpcmtable[index];
        book2 = book1 + 8;
        code >>= 4;                             // upper nibble is scale
        vscale = (0x8000 >> ((12 - code) - 1)); // very strange. 0x8000 would be .5 in 16:16 format
        // so this appears to be a fractional scale based
        // on the 12 based inverse of the scale value.  note
        // that this could be negative, in which case we do
        // not use the calculated vscale value... see the
        // if(code>12) check below

        inPtr++; // coded adpcm data lies next
        j = 0;
        while (j < 8) // loop of 8, for 8 coded nibbles from 4 bytes
        // which yields 8 int16_t pcm values
        {
            icode = BufferSpace[(AudioInBuffer + inPtr) ^ 3];
            inPtr++;

            inp1[j] = (int16_t)((icode & 0xf0) << 8); // this will in effect be signed
            if (code < 12)
                inp1[j] = ((int32_t)((int32_t)inp1[j] * (int32_t)vscale) >> 16);
            else
                int32_t catchme = 1;
            j++;

            inp1[j] = (int16_t)((icode & 0xf) << 12);
            if (code < 12)
                inp1[j] = ((int32_t)((int32_t)inp1[j] * (int32_t)vscale) >> 16);
            else
                int32_t catchme = 1;
            j++;
        }
        j = 0;
        while (j < 8)
        {
            icode = BufferSpace[(AudioInBuffer + inPtr) ^ 3];
            inPtr++;

            inp2[j] = (int16_t)((icode & 0xf0) << 8); // this will in effect be signed
            if (code < 12)
                inp2[j] = ((int32_t)((int32_t)inp2[j] * (int32_t)vscale) >> 16);
            else
                int32_t catchme = 1;
            j++;

            inp2[j] = (int16_t)((icode & 0xf) << 12);
            if (code < 12)
                inp2[j] = ((int32_t)((int32_t)inp2[j] * (int32_t)vscale) >> 16);
            else
                int32_t catchme = 1;
            j++;
        }

        a[0] = (int32_t)book1[0] * (int32_t)l1;
        a[0] += (int32_t)book2[0] * (int32_t)l2;
        a[0] += (int32_t)inp1[0] * (int32_t)2048;

        a[1] = (int32_t)book1[1] * (int32_t)l1;
        a[1] += (int32_t)book2[1] * (int32_t)l2;
        a[1] += (int32_t)book2[0] * inp1[0];
        a[1] += (int32_t)inp1[1] * (int32_t)2048;

        a[2] = (int32_t)book1[2] * (int32_t)l1;
        a[2] += (int32_t)book2[2] * (int32_t)l2;
        a[2] += (int32_t)book2[1] * inp1[0];
        a[2] += (int32_t)book2[0] * inp1[1];
        a[2] += (int32_t)inp1[2] * (int32_t)2048;

        a[3] = (int32_t)book1[3] * (int32_t)l1;
        a[3] += (int32_t)book2[3] * (int32_t)l2;
        a[3] += (int32_t)book2[2] * inp1[0];
        a[3] += (int32_t)book2[1] * inp1[1];
        a[3] += (int32_t)book2[0] * inp1[2];
        a[3] += (int32_t)inp1[3] * (int32_t)2048;

        a[4] = (int32_t)book1[4] * (int32_t)l1;
        a[4] += (int32_t)book2[4] * (int32_t)l2;
        a[4] += (int32_t)book2[3] * inp1[0];
        a[4] += (int32_t)book2[2] * inp1[1];
        a[4] += (int32_t)book2[1] * inp1[2];
        a[4] += (int32_t)book2[0] * inp1[3];
        a[4] += (int32_t)inp1[4] * (int32_t)2048;

        a[5] = (int32_t)book1[5] * (int32_t)l1;
        a[5] += (int32_t)book2[5] * (int32_t)l2;
        a[5] += (int32_t)book2[4] * inp1[0];
        a[5] += (int32_t)book2[3] * inp1[1];
        a[5] += (int32_t)book2[2] * inp1[2];
        a[5] += (int32_t)book2[1] * inp1[3];
        a[5] += (int32_t)book2[0] * inp1[4];
        a[5] += (int32_t)inp1[5] * (int32_t)2048;

        a[6] = (int32_t)book1[6] * (int32_t)l1;
        a[6] += (int32_t)book2[6] * (int32_t)l2;
        a[6] += (int32_t)book2[5] * inp1[0];
        a[6] += (int32_t)book2[4] * inp1[1];
        a[6] += (int32_t)book2[3] * inp1[2];
        a[6] += (int32_t)book2[2] * inp1[3];
        a[6] += (int32_t)book2[1] * inp1[4];
        a[6] += (int32_t)book2[0] * inp1[5];
        a[6] += (int32_t)inp1[6] * (int32_t)2048;

        a[7] = (int32_t)book1[7] * (int32_t)l1;
        a[7] += (int32_t)book2[7] * (int32_t)l2;
        a[7] += (int32_t)book2[6] * inp1[0];
        a[7] += (int32_t)book2[5] * inp1[1];
        a[7] += (int32_t)book2[4] * inp1[2];
        a[7] += (int32_t)book2[3] * inp1[3];
        a[7] += (int32_t)book2[2] * inp1[4];
        a[7] += (int32_t)book2[1] * inp1[5];
        a[7] += (int32_t)book2[0] * inp1[6];
        a[7] += (int32_t)inp1[7] * (int32_t)2048;

        for (j = 0; j < 8; j++)
        {
            a[j ^ 1] >>= 11;
            if (a[j ^ 1] > 32767)
                a[j ^ 1] = 32767;
            else if (a[j ^ 1] < -32768)
                a[j ^ 1] = -32768;
            *(out++) = a[j ^ 1];
        }
        l1 = a[6];
        l2 = a[7];

        a[0] = (int32_t)book1[0] * (int32_t)l1;
        a[0] += (int32_t)book2[0] * (int32_t)l2;
        a[0] += (int32_t)inp2[0] * (int32_t)2048;

        a[1] = (int32_t)book1[1] * (int32_t)l1;
        a[1] += (int32_t)book2[1] * (int32_t)l2;
        a[1] += (int32_t)book2[0] * inp2[0];
        a[1] += (int32_t)inp2[1] * (int32_t)2048;

        a[2] = (int32_t)book1[2] * (int32_t)l1;
        a[2] += (int32_t)book2[2] * (int32_t)l2;
        a[2] += (int32_t)book2[1] * inp2[0];
        a[2] += (int32_t)book2[0] * inp2[1];
        a[2] += (int32_t)inp2[2] * (int32_t)2048;

        a[3] = (int32_t)book1[3] * (int32_t)l1;
        a[3] += (int32_t)book2[3] * (int32_t)l2;
        a[3] += (int32_t)book2[2] * inp2[0];
        a[3] += (int32_t)book2[1] * inp2[1];
        a[3] += (int32_t)book2[0] * inp2[2];
        a[3] += (int32_t)inp2[3] * (int32_t)2048;

        a[4] = (int32_t)book1[4] * (int32_t)l1;
        a[4] += (int32_t)book2[4] * (int32_t)l2;
        a[4] += (int32_t)book2[3] * inp2[0];
        a[4] += (int32_t)book2[2] * inp2[1];
        a[4] += (int32_t)book2[1] * inp2[2];
        a[4] += (int32_t)book2[0] * inp2[3];
        a[4] += (int32_t)inp2[4] * (int32_t)2048;

        a[5] = (int32_t)book1[5] * (int32_t)l1;
        a[5] += (int32_t)book2[5] * (int32_t)l2;
        a[5] += (int32_t)book2[4] * inp2[0];
        a[5] += (int32_t)book2[3] * inp2[1];
        a[5] += (int32_t)book2[2] * inp2[2];
        a[5] += (int32_t)book2[1] * inp2[3];
        a[5] += (int32_t)book2[0] * inp2[4];
        a[5] += (int32_t)inp2[5] * (int32_t)2048;

        a[6] = (int32_t)book1[6] * (int32_t)l1;
        a[6] += (int32_t)book2[6] * (int32_t)l2;
        a[6] += (int32_t)book2[5] * inp2[0];
        a[6] += (int32_t)book2[4] * inp2[1];
        a[6] += (int32_t)book2[3] * inp2[2];
        a[6] += (int32_t)book2[2] * inp2[3];
        a[6] += (int32_t)book2[1] * inp2[4];
        a[6] += (int32_t)book2[0] * inp2[5];
        a[6] += (int32_t)inp2[6] * (int32_t)2048;

        a[7] = (int32_t)book1[7] * (int32_t)l1;
        a[7] += (int32_t)book2[7] * (int32_t)l2;
        a[7] += (int32_t)book2[6] * inp2[0];
        a[7] += (int32_t)book2[5] * inp2[1];
        a[7] += (int32_t)book2[4] * inp2[2];
        a[7] += (int32_t)book2[3] * inp2[3];
        a[7] += (int32_t)book2[2] * inp2[4];
        a[7] += (int32_t)book2[1] * inp2[5];
        a[7] += (int32_t)book2[0] * inp2[6];
        a[7] += (int32_t)inp2[7] * (int32_t)2048;

        for (j = 0; j < 8; j++)
        {
            a[j ^ 1] >>= 11;
            if (a[j ^ 1] > 32767)
                a[j ^ 1] = 32767;
            else if (a[j ^ 1] < -32768)
                a[j ^ 1] = -32768;
            *(out++) = a[j ^ 1];
        }
        l1 = a[6];
        l2 = a[7];

        count -= 32;
    }
    out -= 16;
    memcpy(&rsp.rdram[Address], out, 32);
}

static void LOADBUFF()
{
    // memcpy causes static... endianess issue :(
    uint32_t v0;
    uint32_t cnt;
    if (AudioCount == 0) return;
    v0 = (inst2 & 0xfffffc); // + SEGMENTS[(inst2>>24)&0xf];
    memcpy(BufferSpace + (AudioInBuffer & 0xFFFC), rsp.rdram + v0, (AudioCount + 3) & 0xFFFC);
}

static void SAVEBUFF()
{
    // memcpy causes static... endianess issue :(
    uint32_t v0;
    uint32_t cnt;
    if (AudioCount == 0) return;
    v0 = (inst2 & 0xfffffc); // + SEGMENTS[(inst2>>24)&0xf];
    memcpy(rsp.rdram + v0, BufferSpace + (AudioOutBuffer & 0xFFFC), (AudioCount + 3) & 0xFFFC);
}

static void SEGMENT()
{
    // Should work
    SEGMENTS[(inst2 >> 24) & 0xf] = (inst2 & 0xffffff);
}

static void SETBUFF()
{
    // Should work ;-)
    if ((inst1 >> 0x10) & 0x8)
    {
        // A_AUX - Auxillary Sound Buffer Settings
        AudioAuxA = uint16_t(inst1);
        AudioAuxC = uint16_t((inst2 >> 0x10));
        AudioAuxE = uint16_t(inst2);
    }
    else
    {
        // A_MAIN - Main Sound Buffer Settings
        AudioInBuffer = uint16_t(inst1);            // 0x00
        AudioOutBuffer = uint16_t((inst2 >> 0x10)); // 0x02
        AudioCount = uint16_t(inst2);               // 0x04
    }
}

static void DMEMMOVE()
{
    // Doesn't sound just right?... will fix when HLE is ready - 03-11-01
    uint32_t v0, v1;
    uint32_t cnt;
    if ((inst2 & 0xffff) == 0) return;
    v0 = (inst1 & 0xFFFF);
    v1 = (inst2 >> 0x10);
    // assert ((v1 & 0x3) == 0);
    // assert ((v0 & 0x3) == 0);
    uint32_t count = ((inst2 + 3) & 0xfffc);
    // v0 = (v0) & 0xfffc;
    // v1 = (v1) & 0xfffc;

    // memcpy (BufferSpace+v1, BufferSpace+v0, count-1);
    for (cnt = 0; cnt < count; cnt++)
    {
        *(uint8_t *)(BufferSpace + ((cnt + v1) ^ 3)) = *(uint8_t *)(BufferSpace + ((cnt + v0) ^ 3));
    }
}

static void LOADADPCM()
{
    // Loads an ADPCM table - Works 100% Now 03-13-01
    uint32_t v0;
    v0 = (inst2 & 0xffffff); // + SEGMENTS[(inst2>>24)&0xf];
    uint16_t *table = (uint16_t *)(rsp.rdram + v0);
    for (uint32_t x = 0; x < ((inst1 & 0xffff) >> 0x4); x++)
    {
        adpcmtable[0x1 + (x << 3)] = table[0];
        adpcmtable[0x0 + (x << 3)] = table[1];

        adpcmtable[0x3 + (x << 3)] = table[2];
        adpcmtable[0x2 + (x << 3)] = table[3];

        adpcmtable[0x5 + (x << 3)] = table[4];
        adpcmtable[0x4 + (x << 3)] = table[5];

        adpcmtable[0x7 + (x << 3)] = table[6];
        adpcmtable[0x6 + (x << 3)] = table[7];
        table += 8;
    }
}

static void INTERLEAVE()
{
    // Works... - 3-11-01
    uint32_t inL, inR;
    uint16_t *outbuff = (uint16_t *)(AudioOutBuffer + BufferSpace);
    uint16_t *inSrcR;
    uint16_t *inSrcL;
    uint16_t Left, Right;

    inL = inst2 & 0xFFFF;
    inR = (inst2 >> 16) & 0xFFFF;

    inSrcR = (uint16_t *)(BufferSpace + inR);
    inSrcL = (uint16_t *)(BufferSpace + inL);

    for (int32_t x = 0; x < (AudioCount / 4); x++)
    {
        Left = *(inSrcL++);
        Right = *(inSrcR++);

        *(outbuff++) = *(inSrcR++);
        *(outbuff++) = *(inSrcL++);
        *(outbuff++) = (uint16_t)Right;
        *(outbuff++) = (uint16_t)Left;
    }
}

static void MIXER()
{
    // Fixed a sign issue... 03-14-01
    uint32_t dmemin = (uint16_t)(inst2 >> 0x10);
    uint32_t dmemout = (uint16_t)(inst2 & 0xFFFF);
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    int32_t gain = (int16_t)(inst1 & 0xFFFF);
    int32_t temp;

    if (AudioCount == 0) return;

    for (int32_t x = 0; x < AudioCount; x += 2)
    {
        // I think I can do this a lot easier
        temp = (*(int16_t *)(BufferSpace + dmemin + x) * gain) >> 15;
        temp += *(int16_t *)(BufferSpace + dmemout + x);

        if ((int32_t)temp > 32767) temp = 32767;
        if ((int32_t)temp < -32768) temp = -32768;

        *(uint16_t *)(BufferSpace + dmemout + x) = (uint16_t)(temp & 0xFFFF);
    }
}

// TOP Performance Hogs:
// Command: ADPCM    - Calls:  48 - Total Time: 331226 - Avg Time:  6900.54 - Percent: 31.53%
// Command: ENVMIXER - Calls:  48 - Total Time: 408563 - Avg Time:  8511.73 - Percent: 38.90%
// Command: LOADBUFF - Calls:  56 - Total Time:  21551 - Avg Time:   384.84 - Percent:  2.05%
// Command: RESAMPLE - Calls:  48 - Total Time: 225922 - Avg Time:  4706.71 - Percent: 21.51%

// Command: ADPCM    - Calls:  48 - Total Time: 391600 - Avg Time:  8158.33 - Percent: 32.52%
// Command: ENVMIXER - Calls:  48 - Total Time: 444091 - Avg Time:  9251.90 - Percent: 36.88%
// Command: LOADBUFF - Calls:  58 - Total Time:  29945 - Avg Time:   516.29 - Percent:  2.49%
// Command: RESAMPLE - Calls:  48 - Total Time: 276354 - Avg Time:  5757.38 - Percent: 22.95%

void (*ABI1[0x20])() = {
    // TOP Performace Hogs: MIXER, RESAMPLE, ENVMIXER
    SPNOOP,    ADPCM,  CLEARBUFF,  ENVMIXER, LOADBUFF, RESAMPLE, SAVEBUFF, UNKNOWN, SETBUFF, SETVOL, DMEMMOVE,
    LOADADPCM, MIXER,  INTERLEAVE, UNKNOWN,  SETLOOP,  SPNOOP,   SPNOOP,   SPNOOP,  SPNOOP,  SPNOOP, SPNOOP,
    SPNOOP,    SPNOOP, SPNOOP,     SPNOOP,   SPNOOP,   SPNOOP,   SPNOOP,   SPNOOP,  SPNOOP,  SPNOOP};
