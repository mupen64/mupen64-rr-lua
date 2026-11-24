/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "HLE.hpp"
#include "ADPCM.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <tmmintrin.h>

using namespace BuiltinTAS::TASRSP;

static void SPNOOP()
{
    assert(false && std::format("Unknown/Unimplemented Audio Command {} in ABI 3", inst1 >> 24).c_str());
}

extern uint32_t loopval;

extern int16_t Env_Dry;
extern int16_t Env_Wet;
extern int16_t Vol_Left;
extern int16_t Vol_Right;
extern int16_t VolTrg_Left;
extern int32_t VolRamp_Left;
// extern uint16_t VolRate_Left;
extern int16_t VolTrg_Right;
extern int32_t VolRamp_Right;
// extern uint16_t VolRate_Right;

extern int16_t hleMixerWorkArea[256];

extern uint8_t BufferSpace[0x10000];

static void SETVOL3()
{
    uint8_t Flags = (uint8_t)(inst1 >> 0x10);
    if (Flags & 0x4)
    {
        // 288
        if (Flags & 0x2)
        {
            // 290
            Vol_Left = *(int16_t *)&inst1;                   // 0x50
            Env_Dry = (int16_t)(*(int32_t *)&inst2 >> 0x10); // 0x4E
            Env_Wet = *(int16_t *)&inst2;                    // 0x4C
        }
        else
        {
            VolTrg_Right = *(int16_t *)&inst1; // 0x46
            // VolRamp_Right = (uint16_t)(inst2 >> 0x10) | (int32_t)(int16_t)(inst2 << 0x10);
            VolRamp_Right = *(int32_t *)&inst2; // 0x48/0x4A
        }
    }
    else
    {
        VolTrg_Left = *(int16_t *)&inst1;  // 0x40
        VolRamp_Left = *(int32_t *)&inst2; // 0x42/0x44
    }
}

static void ENVMIXER3()
{
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    uint32_t addy = (inst2 & 0xFFFFFF);

    int16_t *inp = (int16_t *)(BufferSpace + 0x4F0);
    int16_t *out = (int16_t *)(BufferSpace + 0x9D0);
    int16_t *aux1 = (int16_t *)(BufferSpace + 0xB40);
    int16_t *aux2 = (int16_t *)(BufferSpace + 0xCB0);
    int16_t *aux3 = (int16_t *)(BufferSpace + 0xE20);
    int32_t LAdder, LAcc, LVol;
    int32_t RAdder, RAcc, RVol;
    int16_t RSig, LSig; // Most significant part of the Ramp Value
    int16_t Wet, Dry;
    int16_t LTrg, RTrg;

    Vol_Right = (*(int16_t *)&inst1);

    if (flags & A_INIT)
    {
        LAdder = VolRamp_Left / 8;
        LAcc = 0;
        LVol = Vol_Left;
        LSig = (int16_t)(VolRamp_Left >> 16);

        RAdder = VolRamp_Right / 8;
        RAcc = 0;
        RVol = Vol_Right;
        RSig = (int16_t)(VolRamp_Right >> 16);

        Wet = (int16_t)Env_Wet;
        Dry = (int16_t)Env_Dry; // Save Wet/Dry values
        LTrg = VolTrg_Left;
        RTrg = VolTrg_Right; // Save Current Left/Right Targets
    }
    else
    {
        memcpy((uint8_t *)hleMixerWorkArea, g_plugin->rdram + addy, 80);
        Wet = *(int16_t *)(hleMixerWorkArea + 0);     // 0-1
        Dry = *(int16_t *)(hleMixerWorkArea + 2);     // 2-3
        LTrg = *(int16_t *)(hleMixerWorkArea + 4);    // 4-5
        RTrg = *(int16_t *)(hleMixerWorkArea + 6);    // 6-7
        LAdder = *(int32_t *)(hleMixerWorkArea + 8);  // 8-9 (hleMixerWorkArea is a 16bit pointer)
        RAdder = *(int32_t *)(hleMixerWorkArea + 10); // 10-11
        LAcc = *(int32_t *)(hleMixerWorkArea + 12);   // 12-13
        RAcc = *(int32_t *)(hleMixerWorkArea + 14);   // 14-15
        LVol = *(int32_t *)(hleMixerWorkArea + 16);   // 16-17
        RVol = *(int32_t *)(hleMixerWorkArea + 18);   // 18-19
        LSig = *(int16_t *)(hleMixerWorkArea + 20);   // 20-21
        RSig = *(int16_t *)(hleMixerWorkArea + 22);   // 22-23
    }

    const auto advance = [](int32_t &acc, int32_t &volume, int32_t adder, int16_t target, int16_t sign) {
        acc += adder;
        volume += acc >> 16;
        acc &= 0xffff;
        volume = sign >= 0 ? std::min<int32_t>(volume, target) : std::max<int32_t>(volume, target);
        return volume;
    };
    const auto mix = [](int16_t *output, __m128i input, __m128i volume, __m128i gain) {
        auto *buffer = reinterpret_cast<__m128i *>(output);
        const auto scaled = _mm_mulhrs_epi16(input, _mm_mulhrs_epi16(volume, gain));
        _mm_storeu_si128(buffer, _mm_adds_epi16(_mm_loadu_si128(buffer), scaled));
    };
    const auto mix_wide = [](int16_t &output, int16_t input, int32_t volume, int16_t gain) {
        const int32_t coefficient = (gain * volume + 0x4000) >> 15;
        const int32_t sample = (input * coefficient + 0x4000) >> 15;
        output = static_cast<int16_t>(std::clamp(output + sample, -32768, 32767));
    };
    const auto dry = _mm_set1_epi16(Dry);
    const auto wet = _mm_set1_epi16(Wet);
    const int32_t min_volume = (Dry == -32768 || Wet == -32768) ? -32767 : -32768;

    for (int32_t y = 0; y < 0x170 / 2; y += 8)
    {
        int32_t left[8], right[8];
        bool fits = true;
        for (int32_t x = 0; x < 8; ++x)
        {
            left[x ^ 1] = advance(LAcc, LVol, LAdder, LTrg, LSig);
            right[x ^ 1] = advance(RAcc, RVol, RAdder, RTrg, RSig);
            fits &= LVol >= min_volume && LVol <= 32767 && RVol >= min_volume && RVol <= 32767;
        }

        if (fits)
        {
            const auto pack = [](const int32_t *volume) {
                return _mm_packs_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i *>(volume)),
                    _mm_loadu_si128(reinterpret_cast<const __m128i *>(volume + 4)));
            };
            const auto left_volume = pack(left);
            const auto right_volume = pack(right);
            const auto input = _mm_loadu_si128(reinterpret_cast<const __m128i *>(inp + y));
            mix(out + y, input, left_volume, dry);
            mix(aux1 + y, input, right_volume, dry);
            mix(aux2 + y, input, left_volume, wet);
            mix(aux3 + y, input, right_volume, wet);
        }
        else
        {
            for (int32_t x = 0; x < 8; ++x)
            {
                mix_wide(out[y + x], inp[y + x], left[x], Dry);
                mix_wide(aux1[y + x], inp[y + x], right[x], Dry);
                mix_wide(aux2[y + x], inp[y + x], left[x], Wet);
                mix_wide(aux3[y + x], inp[y + x], right[x], Wet);
            }
        }
    }

    *(int16_t *)(hleMixerWorkArea + 0) = Wet;     // 0-1
    *(int16_t *)(hleMixerWorkArea + 2) = Dry;     // 2-3
    *(int16_t *)(hleMixerWorkArea + 4) = LTrg;    // 4-5
    *(int16_t *)(hleMixerWorkArea + 6) = RTrg;    // 6-7
    *(int32_t *)(hleMixerWorkArea + 8) = LAdder;  // 8-9 (hleMixerWorkArea is a 16bit pointer)
    *(int32_t *)(hleMixerWorkArea + 10) = RAdder; // 10-11
    *(int32_t *)(hleMixerWorkArea + 12) = LAcc;   // 12-13
    *(int32_t *)(hleMixerWorkArea + 14) = RAcc;   // 14-15
    *(int32_t *)(hleMixerWorkArea + 16) = LVol;   // 16-17
    *(int32_t *)(hleMixerWorkArea + 18) = RVol;   // 18-19
    *(int16_t *)(hleMixerWorkArea + 20) = LSig;   // 20-21
    *(int16_t *)(hleMixerWorkArea + 22) = RSig;   // 22-23
    memcpy(g_plugin->rdram + addy, (uint8_t *)hleMixerWorkArea, 80);
}

//*/
static void ENVMIXER3o()
{
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    uint32_t addy = (inst2 & 0xFFFFFF); // + SEGMENTS[(inst2>>24)&0xf];

    //  ********* Make sure these conditions are met... ***********
    if ((AudioInBuffer | AudioOutBuffer | AudioAuxA | AudioAuxC | AudioAuxE | AudioCount) & 0x3)
    {
        assert(false && "misaligned EnvMixer");
    }

    int16_t *inp = (int16_t *)(BufferSpace + 0x4F0);
    int16_t *out = (int16_t *)(BufferSpace + 0x9D0);
    int16_t *aux1 = (int16_t *)(BufferSpace + 0xB40);
    int16_t *aux2 = (int16_t *)(BufferSpace + 0xCB0);
    int16_t *aux3 = (int16_t *)(BufferSpace + 0xE20);

    int32_t MainR;
    int32_t MainL;
    int32_t AuxR;
    int32_t AuxL;
    int32_t i1, o1, a1, a2, a3;
    uint16_t AuxIncRate = 1;
    int16_t zero[8];
    memset(zero, 0, 16);
    int32_t LVol, RVol;
    int32_t LAcc, RAcc;
    int32_t LTrg, RTrg;
    int16_t Wet, Dry;

    Vol_Right = (*(int16_t *)&inst1);
    if (flags & A_INIT)
    {
        LVol = (((int32_t)(int16_t)Vol_Left * VolRamp_Left) - ((int32_t)(int16_t)Vol_Left << 16)) >> 3;
        RVol = (((int32_t)(int16_t)Vol_Right * VolRamp_Right) - ((int32_t)(int16_t)Vol_Right << 16)) >> 3;
        LAcc = ((int32_t)(int16_t)Vol_Left << 16);
        RAcc = ((int32_t)(int16_t)Vol_Right << 16);
        Wet = Env_Wet;
        Dry = Env_Dry; // Save Wet/Dry values
        LTrg = VolTrg_Left * 0x10000;
        RTrg = VolTrg_Right * 0x10000;
    }
    else
    {
        // Load LVol, RVol, LAcc, and RAcc (all 32bit)
        // Load Wet, Dry, LTrg, RTrg
        memcpy((uint8_t *)hleMixerWorkArea, (g_plugin->rdram + addy), 80);
        Wet = *(int16_t *)(hleMixerWorkArea + 0);   // 0-1
        Dry = *(int16_t *)(hleMixerWorkArea + 2);   // 2-3
        LTrg = *(int32_t *)(hleMixerWorkArea + 4);  // 4-5
        RTrg = *(int32_t *)(hleMixerWorkArea + 6);  // 6-7
        LVol = *(int32_t *)(hleMixerWorkArea + 8);  // 8-9 (hleMixerWorkArea is a 16bit pointer)
        RVol = *(int32_t *)(hleMixerWorkArea + 10); // 10-11
        LAcc = *(int32_t *)(hleMixerWorkArea + 12); // 12-13
        RAcc = *(int32_t *)(hleMixerWorkArea + 14); // 14-15
    }

    if (!(flags & A_AUX))
    {
        AuxIncRate = 0;
        aux2 = aux3 = zero;
    }

    for (int32_t x = 0; x < (0x170 / 2); x++)
    {
        i1 = (int32_t)inp[x ^ 1];
        o1 = (int32_t)out[x ^ 1];
        a1 = (int32_t)aux1[x ^ 1];
        if (AuxIncRate)
        {
            a2 = (int32_t)aux2[x ^ 1];
            a3 = (int32_t)aux3[x ^ 1];
        }
        // TODO: here...
        // LAcc = (LTrg << 16);
        // RAcc = (RTrg << 16);

        LAcc += LVol;
        RAcc += RVol;

        if (LVol < 0)
        {
            // Decrementing
            if (LAcc < LTrg)
            {
                LAcc = LTrg;
                LVol = 0;
            }
        }
        else
        {
            if (LAcc > LTrg)
            {
                LAcc = LTrg;
                LVol = 0;
            }
        }

        if (RVol < 0)
        {
            // Decrementing
            if (RAcc < RTrg)
            {
                RAcc = RTrg;
                RVol = 0;
            }
        }
        else
        {
            if (RAcc > RTrg)
            {
                RAcc = RTrg;
                RVol = 0;
            }
        }

        MainL = ((Dry * (LAcc >> 16)) + 0x4000) >> 15;
        MainR = ((Dry * (RAcc >> 16)) + 0x4000) >> 15;
        AuxL = ((Wet * (LAcc >> 16)) + 0x4000) >> 15;
        AuxR = ((Wet * (RAcc >> 16)) + 0x4000) >> 15;

        o1 += (/*(o1*0x7fff)+*/ (i1 * MainR) + 0x4000) >> 15;

        a1 += (/*(a1*0x7fff)+*/ (i1 * MainL) + 0x4000) >> 15;

        if (o1 > 32767)
            o1 = 32767;
        else if (o1 < -32768)
            o1 = -32768;

        if (a1 > 32767)
            a1 = 32767;
        else if (a1 < -32768)
            a1 = -32768;

        out[x ^ 1] = o1;
        aux1[x ^ 1] = a1;
        if (AuxIncRate)
        {
            a2 += (/*(a2*0x7fff)+*/ (i1 * AuxR) + 0x4000) >> 15;
            a3 += (/*(a3*0x7fff)+*/ (i1 * AuxL) + 0x4000) >> 15;

            if (a2 > 32767)
                a2 = 32767;
            else if (a2 < -32768)
                a2 = -32768;

            if (a3 > 32767)
                a3 = 32767;
            else if (a3 < -32768)
                a3 = -32768;

            aux2[x ^ 1] = a2;
            aux3[x ^ 1] = a3;
        }
    }

    *(int16_t *)(hleMixerWorkArea + 0) = Wet;   // 0-1
    *(int16_t *)(hleMixerWorkArea + 2) = Dry;   // 2-3
    *(int32_t *)(hleMixerWorkArea + 4) = LTrg;  // 4-5
    *(int32_t *)(hleMixerWorkArea + 6) = RTrg;  // 6-7
    *(int32_t *)(hleMixerWorkArea + 8) = LVol;  // 8-9 (hleMixerWorkArea is a 16bit pointer)
    *(int32_t *)(hleMixerWorkArea + 10) = RVol; // 10-11
    *(int32_t *)(hleMixerWorkArea + 12) = LAcc; // 12-13
    *(int32_t *)(hleMixerWorkArea + 14) = RAcc; // 14-15
    memcpy(g_plugin->rdram + addy, (uint8_t *)hleMixerWorkArea, 80);
}

static void CLEARBUFF3()
{
    uint16_t addr = (uint16_t)(inst1 & 0xffff);
    uint16_t count = (uint16_t)(inst2 & 0xffff);
    memset(BufferSpace + addr + 0x4f0, 0, count);
}

static void MIXER3()
{
    // Needs accuracy verification...
    uint16_t dmemin = (uint16_t)(inst2 >> 0x10) + 0x4f0;
    uint16_t dmemout = (uint16_t)(inst2 & 0xFFFF) + 0x4f0;
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    int32_t gain = (int16_t)(inst1 & 0xFFFF) * 2;
    int32_t temp;

    for (int32_t x = 0; x < 0x170; x += 2)
    {
        // I think I can do this a lot easier
        temp = (*(int16_t *)(BufferSpace + dmemin + x) * gain) >> 16;
        temp += *(int16_t *)(BufferSpace + dmemout + x);

        if ((int32_t)temp > 32767) temp = 32767;
        if ((int32_t)temp < -32768) temp = -32768;

        *(uint16_t *)(BufferSpace + dmemout + x) = (uint16_t)(temp & 0xFFFF);
    }
}

static void LOADBUFF3()
{
    uint32_t v0;
    uint32_t cnt = (((inst1 >> 0xC) + 3) & 0xFFC);
    v0 = (inst2 & 0xfffffc);
    uint32_t src = (inst1 & 0xffc) + 0x4f0;
    memcpy(BufferSpace + src, g_plugin->rdram + v0, cnt);
}

static void SAVEBUFF3()
{
    uint32_t v0;
    uint32_t cnt = (((inst1 >> 0xC) + 3) & 0xFFC);
    v0 = (inst2 & 0xfffffc);
    uint32_t src = (inst1 & 0xffc) + 0x4f0;
    memcpy(g_plugin->rdram + v0, BufferSpace + src, cnt);
}

static void LOADADPCM3()
{
    // Loads an ADPCM table - Works 100% Now 03-13-01
    uint32_t v0;
    v0 = (inst2 & 0xffffff);
    // memcpy (dmem+0x3f0, rsp->rdram+v0, inst1&0xffff);
    // assert ((inst1&0xffff) <= 0x80);
    uint16_t *table = (uint16_t *)(g_plugin->rdram + v0);
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

static void DMEMMOVE3()
{
    // Needs accuracy verification...
    uint32_t v0, v1;
    uint32_t cnt;
    v0 = (inst1 & 0xFFFF) + 0x4f0;
    v1 = (inst2 >> 0x10) + 0x4f0;
    uint32_t count = ((inst2 + 3) & 0xfffc);

    // memcpy (dmem+v1, dmem+v0, count-1);
    for (cnt = 0; cnt < count; cnt++)
    {
        *(uint8_t *)(BufferSpace + ((cnt + v1) ^ 3)) = *(uint8_t *)(BufferSpace + ((cnt + v0) ^ 3));
    }
}

static void SETLOOP3()
{
    loopval = (inst2 & 0xffffff);
}

static void ADPCM3()
{
    const uint8_t flags = inst2 >> 28;
    const uint32_t address = inst1 & 0xffffff;
    decode_adpcm<4>(BufferSpace, 0x4f0, (inst2 >> 12) & 0xf, (inst2 & 0xfff) + 0x4f0, (inst2 >> 16) & 0xfff, flags,
        g_plugin->rdram, address, loopval);
}

static void RESAMPLE3()
{
    resample_audio_per_tap(inst1 & 0xffffff, ((inst2 >> 14) & 0xffff) << 1, (((inst2 >> 2) & 0xfff) + 0x4f0) / 2,
        (inst2 & 0x3 ? 0x660 : 0x4f0) / 2, 0x170 / 2, inst2 & 0x40000000);
}

static void INTERLEAVE3()
{
    // Needs accuracy verification...
    // uint32_t inL, inR;
    uint16_t *outbuff = (uint16_t *)(BufferSpace + 0x4f0); //(uint16_t *)(AudioOutBuffer+dmem);
    uint16_t *inSrcR;
    uint16_t *inSrcL;
    uint16_t Left, Right;

    // inR = inst2 & 0xFFFF;
    // inL = (inst2 >> 16) & 0xFFFF;

    inSrcR = (uint16_t *)(BufferSpace + 0xb40);
    inSrcL = (uint16_t *)(BufferSpace + 0x9d0);

    for (int32_t x = 0; x < (0x170 / 4); x++)
    {
        Left = *(inSrcL++);
        Right = *(inSrcR++);

        *(outbuff++) = *(inSrcR++);
        *(outbuff++) = *(inSrcL++);
        *(outbuff++) = (uint16_t)Right;
        *(outbuff++) = (uint16_t)Left;
        /*
                Left=*(inSrcL++);
                Right=*(inSrcR++);
                *(outbuff++)=(uint16_t)Left;
                Left >>= 16;
                *(outbuff++)=(uint16_t)Right;
                Right >>= 16;
                *(outbuff++)=(uint16_t)Left;
                *(outbuff++)=(uint16_t)Right;*/
    }
}

// static void UNKNOWN ();
/*
typedef struct {
    uint8_t sync;

    uint8_t error_protection	: 1;	//  0=yes, 1=no
    uint8_t lay				: 2;	// 4-lay = layerI, II or III
    uint8_t version			: 1;	// 3=mpeg 1.0, 2=mpeg 2.5 0=mpeg 2.0
    uint8_t sync2				: 4;

    uint8_t extension			: 1;    // Unknown
    uint8_t padding			: 1;    // padding
    uint8_t sampling_freq		: 2;	// see table below
    uint8_t bitrate_index		: 4;	//     see table below

    uint8_t emphasis			: 2;	//see table below
    uint8_t original			: 1;	// 0=no 1=yes
    uint8_t copyright			: 1;	// 0=no 1=yes
    uint8_t mode_ext			: 2;    // used with "joint stereo" mode
    uint8_t mode				: 2;    // Channel Mode
} mp3struct;

mp3struct mp3;
FILE *mp3dat;
*/

static void WHATISTHIS()
{
}

// static FILE *fp = fopen ("d:\\mp3info.txt", "wt");
uint32_t setaddr;

static void MP3ADDY()
{
    setaddr = (inst2 & 0xffffff);
    //__asm int32_t 3;
    // fprintf (fp, "mp3addy: inst1: %08X, inst2: %08X, loopval: %08X\n", inst1, inst2, loopval);
}

extern "C"
{
    void rsp_run();
    void mp3setup(uint32_t inst1, uint32_t inst2, uint32_t t8);
}

extern uint32_t base, dmembase;

extern "C"
{
    extern char *pDMEM;
}

/*
 {
//	return;
    // Setup Registers...
    mp3setup (inst1, inst2, 0xFA0);

    // Setup Memory Locations...
    //uint32_t base = ((uint32_t*)dmem)[0xFD0/4]; // Should be 000291A0
    memcpy (BufferSpace, dmembase+rsp.rdram, 0x10);
    ((uint32_t*)BufferSpace)[0x0] = base;
    ((uint32_t*)BufferSpace)[0x008/4] += base;
    ((uint32_t*)BufferSpace)[0xFFC/4] = loopval;
    ((uint32_t*)BufferSpace)[0xFF8/4] = dmembase;
    //__asm int32_t 3;
    memcpy (imem+0x238, rsp.rdram+((uint32_t*)BufferSpace)[0x008/4], 0x9C0);
    ((uint32_t*)BufferSpace)[0xFF4/4] = setaddr;
    pDMEM = (char *)BufferSpace;
    rsp_run ();
    dmembase = ((uint32_t*)BufferSpace)[0xFF8/4];
    loopval  = ((uint32_t*)BufferSpace)[0xFFC/4];
//0x1A98  SW       S1, 0x0FF4 (R0)
//0x1A9C  SW       S0, 0x0FF8 (R0)
//0x1AA0  SW       T7, 0x0FFC (R0)
//0x1AA4  SW       T3, 0x0FF0 (R0)
    //fprintf (fp, "mp3: inst1: %08X, inst2: %08X\n", inst1, inst2);
}*/
/*
FFT = Fast Fourier Transform
DCT = Discrete Cosine Transform
MPEG-1 Layer 3 retains Layer 2�s 1152-sample window, as well as the FFT polyphase filter for
backward compatibility, but adds a modified DCT filter. DCT�s advantages over DFTs (discrete
Fourier transforms) include half as many multiply-accumulate operations and half the
generated coefficients because the sinusoidal portion of the calculation is absent, and DCT
generally involves simpler math. The finite lengths of a conventional DCTs� bandpass impulse
responses, however, may result in block-boundary effects. MDCTs overlap the analysis blocks
and lowpass-filter the decoded audio to remove aliases, eliminating these effects. MDCTs also
have a higher transform coding gain than the standard DCT, and their basic functions
correspond to better bandpass response.

MPEG-1 Layer 3�s DCT sub-bands are unequally sized, and correspond to the human auditory
system�s critical bands. In Layer 3 decoders must support both constant- and variable-bit-rate
bit streams. (However, many Layer 1 and 2 decoders also handle variable bit rates). Finally,
Layer 3 encoders Huffman-code the quantized coefficients before archiving or transmission for
additional lossless compression. Bit streams range from 32 to 320 kbps, and 128-kbps rates
achieve near-CD quality, an important specification to enable dual-channel ISDN
(integrated-services-digital-network) to be the future high-bandwidth pipe to the home.

*/
static void DISABLE()
{
}

void (*ABI3[0x20])() = {DISABLE, ADPCM3, CLEARBUFF3, ENVMIXER3, LOADBUFF3, RESAMPLE3, SAVEBUFF3, MP3, MP3ADDY, SETVOL3,
    DMEMMOVE3, LOADADPCM3, MIXER3, INTERLEAVE3, WHATISTHIS, SETLOOP3, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP,
    SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP};
#if 0
void (*ABI3[32])(void) =
{
   SPNOOP  , ADPCM3    , CLEARBUFF3, SPNOOP   ,
   MIXER3   , RESAMPLE3  , SPNOOP   , MP3   ,
   MP3ADDY , SETVOL3   , DMEMMOVE3 , LOADADPCM3,
   MIXER3   , INTERLEAVE3, WHATISTHIS    , SETLOOP3  ,
   SPNOOP  , /*MEMHALVE  , ENVSET1*/ SPNOOP, SPNOOP  , ENVMIXER3 ,
   LOADBUFF3, SAVEBUFF3  , /*ENVSET2*/SPNOOP  , SPNOOP   ,
   SPNOOP  , SPNOOP    , SPNOOP   , SPNOOP   ,
   SPNOOP  , SPNOOP    , SPNOOP   , SPNOOP
};
#endif
