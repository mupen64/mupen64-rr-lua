/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "HLE.hpp"
#include "ADPCM.hpp"

#include <cassert>
#include <cstring>
#include <emmintrin.h>

using namespace BuiltinTAS::TASRSP;

extern uint8_t BufferSpace[0x10000];

static void SPNOOP()
{
    assert(false && std::format("Unknown/Unimplemented Audio Command {} in ABI 2", inst1 >> 24).c_str());
}

extern uint16_t AudioInBuffer;  // 0x0000(T8)
extern uint16_t AudioOutBuffer; // 0x0002(T8)
extern uint16_t AudioCount;     // 0x0004(T8)
extern uint32_t loopval;        // 0x0010(T8)
extern uint32_t SEGMENTS[0x10];

bool isMKABI = false;
bool isZeldaABI = false;

static void LOADADPCM2()
{
    // Loads an ADPCM table - Works 100% Now 03-13-01
    uint32_t v0;
    v0 = (inst2 & 0xffffff);                              // + SEGMENTS[(inst2>>24)&0xf];
    uint16_t *table = (uint16_t *)(g_plugin->rdram + v0); // Zelda2 Specific...

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

static void SETLOOP2()
{
    loopval = inst2 & 0xffffff; // No segment?
}

static void SETBUFF2()
{
    AudioInBuffer = uint16_t(inst1);            // 0x00
    AudioOutBuffer = uint16_t((inst2 >> 0x10)); // 0x02
    AudioCount = uint16_t(inst2);               // 0x04
}

static void ADPCM2()
{
    const uint8_t flags = inst1 >> 16;
    const uint32_t address = inst2 & 0xffffff;
    const int16_t count = static_cast<int16_t>(AudioCount);
    if (flags & 4)
        decode_adpcm<2>(BufferSpace, AudioInBuffer, 0, AudioOutBuffer, count, flags, g_plugin->rdram, address, loopval);
    else
        decode_adpcm<4>(BufferSpace, AudioInBuffer, 0, AudioOutBuffer, count, flags, g_plugin->rdram, address, loopval);
}

static void CLEARBUFF2()
{
    uint16_t addr = (uint16_t)(inst1 & 0xffff);
    uint16_t count = (uint16_t)(inst2 & 0xffff);
    if (count > 0) memset(BufferSpace + addr, 0, count);
}

static void LOADBUFF2()
{
    // Needs accuracy verification...
    uint32_t v0;
    uint32_t cnt = (((inst1 >> 0xC) + 3) & 0xFFC);
    v0 = (inst2 & 0xfffffc); // + SEGMENTS[(inst2>>24)&0xf];
    memcpy(BufferSpace + (inst1 & 0xfffc), g_plugin->rdram + v0, (cnt + 3) & 0xFFFC);
}

static void SAVEBUFF2()
{
    // Needs accuracy verification...
    uint32_t v0;
    uint32_t cnt = (((inst1 >> 0xC) + 3) & 0xFFC);
    v0 = (inst2 & 0xfffffc); // + SEGMENTS[(inst2>>24)&0xf];
    memcpy(g_plugin->rdram + v0, BufferSpace + (inst1 & 0xfffc), (cnt + 3) & 0xFFFC);
}

static void MIXER2()
{
    // Needs accuracy verification...
    uint16_t dmemin = (uint16_t)(inst2 >> 0x10);
    uint16_t dmemout = (uint16_t)(inst2 & 0xFFFF);
    uint32_t count = ((inst1 >> 12) & 0xFF0);
    int32_t gain = (int16_t)(inst1 & 0xFFFF) * 2;
    int32_t temp;

    for (int32_t x = 0; x < count; x += 2)
    {
        // I think I can do this a lot easier

        temp = (*(int16_t *)(BufferSpace + dmemin + x) * gain) >> 16;
        temp += *(int16_t *)(BufferSpace + dmemout + x);

        if ((int32_t)temp > 32767) temp = 32767;
        if ((int32_t)temp < -32768) temp = -32768;

        *(uint16_t *)(BufferSpace + dmemout + x) = (uint16_t)(temp & 0xFFFF);
    }
}

static void RESAMPLE2()
{
    resample_audio_per_tap(inst2 & 0xffffff, (inst1 & 0xffff) << 1, AudioInBuffer / 2, AudioOutBuffer / 2,
        ((AudioCount + 0xf) & 0xfff0) / 2, inst1 & 0x10000);
}

static void DMEMMOVE2()
{
    // Needs accuracy verification...
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

    // memcpy (dmem+v1, dmem+v0, count-1);
    for (cnt = 0; cnt < count; cnt++)
    {
        *(uint8_t *)(BufferSpace + ((cnt + v1) ^ 3)) = *(uint8_t *)(BufferSpace + ((cnt + v0) ^ 3));
    }
}

uint32_t t3, s5, s6;
uint16_t env[8];

static void ENVSETUP1()
{
    uint32_t tmp;

    // fprintf (dfile, "ENVSETUP1: inst1 = %08X, inst2 = %08X\n", inst1, inst2);
    t3 = inst1 & 0xFFFF;
    tmp = (inst1 >> 0x8) & 0xFF00;
    env[4] = (uint16_t)tmp;
    tmp += t3;
    env[5] = (uint16_t)tmp;
    s5 = inst2 >> 0x10;
    s6 = inst2 & 0xFFFF;
    // fprintf (dfile, "	t3 = %X / s5 = %X / s6 = %X / env[4] = %X / env[5] = %X\n", t3, s5, s6, env[4], env[5]);
}

static void ENVSETUP2()
{
    uint32_t tmp;

    // fprintf (dfile, "ENVSETUP2: inst1 = %08X, inst2 = %08X\n", inst1, inst2);
    tmp = (inst2 >> 0x10);
    env[0] = (uint16_t)tmp;
    tmp += s5;
    env[1] = (uint16_t)tmp;
    tmp = inst2 & 0xffff;
    env[2] = (uint16_t)tmp;
    tmp += s6;
    env[3] = (uint16_t)tmp;
    // fprintf (dfile, "	env[0] = %X / env[1] = %X / env[2] = %X / env[3] = %X\n", env[0], env[1], env[2], env[3]);
}

static void ENVMIXER2()
{
    auto *input = reinterpret_cast<int16_t *>(BufferSpace + ((inst1 >> 12) & 0xff0));
    auto *dry_l = reinterpret_cast<int16_t *>(BufferSpace + ((inst2 >> 20) & 0xff0));
    auto *dry_r = reinterpret_cast<int16_t *>(BufferSpace + ((inst2 >> 12) & 0xff0));
    auto *wet_l = reinterpret_cast<int16_t *>(BufferSpace + ((inst2 >> 4) & 0xff0));
    auto *wet_r = reinterpret_cast<int16_t *>(BufferSpace + ((inst2 << 4) & 0xff0));
    const auto dry_l_mask = _mm_set1_epi16(-static_cast<int16_t>((inst1 & 0x2) >> 1));
    const auto dry_r_mask = _mm_set1_epi16(-static_cast<int16_t>(inst1 & 0x1));
    const auto wet_l_mask = _mm_set1_epi16(-static_cast<int16_t>((inst1 & 0x8) >> 1));
    const auto wet_r_mask = _mm_set1_epi16(-static_cast<int16_t>((inst1 & 0x4) >> 1));

    int32_t count = (inst1 >> 8) & 0xff;
    const int32_t adder = isMKABI ? 8 : 16;
    if (isMKABI)
    {
        inst1 = 0;
        t3 = 0;
    }
    else
    {
        s5 *= 2;
        s6 *= 2;
        t3 *= 2;
    }

    const bool swap_wet = inst1 & 0x10;
    const auto scale = [](__m128i samples, uint16_t gain, __m128i mask) {
        const auto gains = _mm_set1_epi16(static_cast<int16_t>(gain));
        const auto correction = _mm_and_si128(_mm_srai_epi16(samples, 15), gains);
        return _mm_xor_si128(_mm_sub_epi16(_mm_mulhi_epu16(samples, gains), correction), mask);
    };
    const auto accumulate = [](int16_t *buffer, __m128i samples) {
        auto *vector = reinterpret_cast<__m128i *>(buffer);
        _mm_storeu_si128(vector, _mm_adds_epi16(_mm_loadu_si128(vector), samples));
    };

    while (count > 0)
    {
        for (int32_t offset = 0; offset < adder; offset += 8)
        {
            const auto channel = offset / 8;
            const auto samples = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + offset));
            const auto left = scale(samples, env[channel], dry_l_mask);
            const auto right = scale(samples, env[channel + 2], dry_r_mask);
            const auto wet_left = scale(left, env[channel + 4], wet_l_mask);
            const auto wet_right = scale(right, env[channel + 4], wet_r_mask);
            accumulate(dry_l + offset, left);
            accumulate(dry_r + offset, right);
            accumulate(wet_l + offset, swap_wet ? wet_right : wet_left);
            accumulate(wet_r + offset, swap_wet ? wet_left : wet_right);
        }

        dry_l += adder;
        dry_r += adder;
        wet_l += adder;
        wet_r += adder;
        input += adder;
        count -= adder;
        env[0] += (uint16_t)s5;
        env[1] += (uint16_t)s5;
        env[2] += (uint16_t)s6;
        env[3] += (uint16_t)s6;
        env[4] += (uint16_t)t3;
        env[5] += (uint16_t)t3;
    }
}

static void DUPLICATE2()
{
    uint16_t Count = (inst1 >> 16) & 0xff;
    uint16_t In = inst1 & 0xffff;
    uint16_t Out = (inst2 >> 16);

    uint16_t buff[64];

    memcpy(buff, BufferSpace + In, 128);

    while (Count)
    {
        memcpy(BufferSpace + Out, buff, 128);
        Out += 128;
        Count--;
    }
}

/*
static void INTERL2 () { // Make your own...
    int16_t Count = inst1 & 0xffff;
    uint16_t  Out   = inst2 & 0xffff;
    uint16_t In     = (inst2 >> 16);

    int16_t *src,*dst,tmp;
    src=(int16_t *)&BufferSpace[In];
    dst=(int16_t *)&BufferSpace[Out];
    while(Count)
    {
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        *(dst++)=*(src++);
        src++;
        Count-=8;
    }
}
*/

static void INTERL2()
{
    int16_t Count = inst1 & 0xffff;
    uint16_t Out = inst2 & 0xffff;
    uint16_t In = (inst2 >> 16);

    uint8_t *src, *dst, tmp;
    src = (uint8_t *)(BufferSpace); //[In];
    dst = (uint8_t *)(BufferSpace); //[Out];
    while (Count)
    {
        *(int16_t *)(dst + (Out ^ 3)) = *(int16_t *)(src + (In ^ 3));
        Out += 2;
        In += 4;
        Count--;
    }
}

static void INTERLEAVE2()
{
    // Needs accuracy verification...
    uint32_t inL, inR;
    uint16_t *outbuff;
    uint16_t *inSrcR;
    uint16_t *inSrcL;
    uint16_t Left, Right;
    uint32_t count;
    count = ((inst1 >> 12) & 0xFF0);
    if (count == 0)
    {
        outbuff = (uint16_t *)(AudioOutBuffer + BufferSpace);
        count = AudioCount;
    }
    else
    {
        outbuff = (uint16_t *)((inst1 & 0xFFFF) + BufferSpace);
    }

    inR = inst2 & 0xFFFF;
    inL = (inst2 >> 16) & 0xFFFF;

    inSrcR = (uint16_t *)(BufferSpace + inR);
    inSrcL = (uint16_t *)(BufferSpace + inL);

    for (uint32_t x = 0; x < (count / 4); x++)
    {
        Left = *(inSrcL++);
        Right = *(inSrcR++);

        *(outbuff++) = *(inSrcR++);
        *(outbuff++) = *(inSrcL++);
        *(outbuff++) = (uint16_t)Right;
        *(outbuff++) = (uint16_t)Left;
    }
}

static void ADDMIXER()
{
    int16_t Count = (inst1 >> 12) & 0x00ff0;
    uint16_t InBuffer = (inst2 >> 16);
    uint16_t OutBuffer = inst2 & 0xffff;

    int16_t *inp, *outp;
    int32_t temp;
    inp = (int16_t *)(BufferSpace + InBuffer);
    outp = (int16_t *)(BufferSpace + OutBuffer);
    for (int32_t cntr = 0; cntr < Count; cntr += 2)
    {
        temp = *outp + *inp;
        if (temp > 32767) temp = 32767;
        if (temp < -32768) temp = -32768;
        outp++;
        inp++;
    }
}

static void HILOGAIN()
{
    uint16_t cnt = inst1 & 0xffff;
    uint16_t out = (inst2 >> 16) & 0xffff;
    int16_t hi = (int16_t)((inst1 >> 4) & 0xf000);
    uint16_t lo = (inst1 >> 20) & 0xf;
    int16_t *src;

    src = (int16_t *)(BufferSpace + out);
    int32_t tmp, val;

    while (cnt)
    {
        val = (int32_t)*src;
        // tmp = ((val * (int32_t)hi) + ((uint64_t)(val * lo) << 16) >> 16);
        tmp = ((val * (int32_t)hi) >> 16) + (uint32_t)(val * lo);
        if ((int32_t)tmp > 32767)
            tmp = 32767;
        else if ((int32_t)tmp < -32768)
            tmp = -32768;
        *src = tmp;
        src++;
        cnt -= 2;
    }
}

static void FILTER2()
{
    static int32_t cnt = 0;
    static int16_t *lutt6;
    static int16_t *lutt5;
    uint8_t *save = (g_plugin->rdram + (inst2 & 0xFFFFFF));
    uint8_t t4 = (uint8_t)((inst1 >> 0x10) & 0xFF);
    int32_t x;

    if (t4 > 1)
    {
        // Then set the cnt variable
        cnt = (inst1 & 0xFFFF);
        lutt6 = (int16_t *)save;
        //				memcpy (dmem+0xFE0, rsp.rdram+(inst2&0xFFFFFF), 0x10);
        return;
    }

    if (t4 == 0)
    {
        //				memcpy (dmem+0xFB0, rsp.rdram+(inst2&0xFFFFFF), 0x20);
        lutt5 = (int16_t *)(save + 0x10);
    }

    lutt5 = (int16_t *)(save + 0x10);

    //			lutt5 = (int16_t *)(dmem + 0xFC0);
    //			lutt6 = (int16_t *)(dmem + 0xFE0);
    for (x = 0; x < 8; x++)
    {
        int32_t a;
        a = (lutt5[x] + lutt6[x]) >> 1;
        lutt5[x] = lutt6[x] = (int16_t)a;
    }
    int16_t *inp1, *inp2;
    int32_t out1[8];
    int16_t outbuff[0x3c0], *outp;
    uint32_t inPtr = (uint32_t)(inst1 & 0xffff);
    inp1 = (int16_t *)(save);
    outp = outbuff;
    inp2 = (int16_t *)(BufferSpace + inPtr);
    for (x = 0; x < cnt; x += 0x10)
    {
        out1[1] = inp1[0] * lutt6[6];
        out1[1] += inp1[3] * lutt6[7];
        out1[1] += inp1[2] * lutt6[4];
        out1[1] += inp1[5] * lutt6[5];
        out1[1] += inp1[4] * lutt6[2];
        out1[1] += inp1[7] * lutt6[3];
        out1[1] += inp1[6] * lutt6[0];
        out1[1] += inp2[1] * lutt6[1]; // 1

        out1[0] = inp1[3] * lutt6[6];
        out1[0] += inp1[2] * lutt6[7];
        out1[0] += inp1[5] * lutt6[4];
        out1[0] += inp1[4] * lutt6[5];
        out1[0] += inp1[7] * lutt6[2];
        out1[0] += inp1[6] * lutt6[3];
        out1[0] += inp2[1] * lutt6[0];
        out1[0] += inp2[0] * lutt6[1];

        out1[3] = inp1[2] * lutt6[6];
        out1[3] += inp1[5] * lutt6[7];
        out1[3] += inp1[4] * lutt6[4];
        out1[3] += inp1[7] * lutt6[5];
        out1[3] += inp1[6] * lutt6[2];
        out1[3] += inp2[1] * lutt6[3];
        out1[3] += inp2[0] * lutt6[0];
        out1[3] += inp2[3] * lutt6[1];

        out1[2] = inp1[5] * lutt6[6];
        out1[2] += inp1[4] * lutt6[7];
        out1[2] += inp1[7] * lutt6[4];
        out1[2] += inp1[6] * lutt6[5];
        out1[2] += inp2[1] * lutt6[2];
        out1[2] += inp2[0] * lutt6[3];
        out1[2] += inp2[3] * lutt6[0];
        out1[2] += inp2[2] * lutt6[1];

        out1[5] = inp1[4] * lutt6[6];
        out1[5] += inp1[7] * lutt6[7];
        out1[5] += inp1[6] * lutt6[4];
        out1[5] += inp2[1] * lutt6[5];
        out1[5] += inp2[0] * lutt6[2];
        out1[5] += inp2[3] * lutt6[3];
        out1[5] += inp2[2] * lutt6[0];
        out1[5] += inp2[5] * lutt6[1];

        out1[4] = inp1[7] * lutt6[6];
        out1[4] += inp1[6] * lutt6[7];
        out1[4] += inp2[1] * lutt6[4];
        out1[4] += inp2[0] * lutt6[5];
        out1[4] += inp2[3] * lutt6[2];
        out1[4] += inp2[2] * lutt6[3];
        out1[4] += inp2[5] * lutt6[0];
        out1[4] += inp2[4] * lutt6[1];

        out1[7] = inp1[6] * lutt6[6];
        out1[7] += inp2[1] * lutt6[7];
        out1[7] += inp2[0] * lutt6[4];
        out1[7] += inp2[3] * lutt6[5];
        out1[7] += inp2[2] * lutt6[2];
        out1[7] += inp2[5] * lutt6[3];
        out1[7] += inp2[4] * lutt6[0];
        out1[7] += inp2[7] * lutt6[1];

        out1[6] = inp2[1] * lutt6[6];
        out1[6] += inp2[0] * lutt6[7];
        out1[6] += inp2[3] * lutt6[4];
        out1[6] += inp2[2] * lutt6[5];
        out1[6] += inp2[5] * lutt6[2];
        out1[6] += inp2[4] * lutt6[3];
        out1[6] += inp2[7] * lutt6[0];
        out1[6] += inp2[6] * lutt6[1];
        outp[1] = /*CLAMP*/ ((out1[1] + 0x4000) >> 0xF);
        outp[0] = /*CLAMP*/ ((out1[0] + 0x4000) >> 0xF);
        outp[3] = /*CLAMP*/ ((out1[3] + 0x4000) >> 0xF);
        outp[2] = /*CLAMP*/ ((out1[2] + 0x4000) >> 0xF);
        outp[5] = /*CLAMP*/ ((out1[5] + 0x4000) >> 0xF);
        outp[4] = /*CLAMP*/ ((out1[4] + 0x4000) >> 0xF);
        outp[7] = /*CLAMP*/ ((out1[7] + 0x4000) >> 0xF);
        outp[6] = /*CLAMP*/ ((out1[6] + 0x4000) >> 0xF);
        inp1 = inp2;
        inp2 += 8;
        outp += 8;
    }
    //			memcpy (rsp.rdram+(inst2&0xFFFFFF), dmem+0xFB0, 0x20);
    memcpy(save, inp2 - 8, 0x10);
    memcpy(BufferSpace + (inst1 & 0xffff), outbuff, cnt);
}

static void SEGMENT2()
{
    if (isZeldaABI)
    {
        FILTER2();
        return;
    }
    if ((inst1 & 0xffffff) == 0)
    {
        isMKABI = true;
        // SEGMENTS[(inst2>>24)&0xf] = (inst2 & 0xffffff);
    }
    else
    {
        isMKABI = false;
        isZeldaABI = true;
        FILTER2();
    }
}

static void UNKNOWN()
{
}

/*
void (*ABI2[0x20])() = {
    SPNOOP, ADPCM2, CLEARBUFF2, SPNOOP, SPNOOP, RESAMPLE2, SPNOOP, SEGMENT2,
    SETBUFF2, SPNOOP, DMEMMOVE2, LOADADPCM2, MIXER2, INTERLEAVE2, HILOGAIN, SETLOOP2,
    SPNOOP, INTERL2, ENVSETUP1, ENVMIXER2, LOADBUFF2, SAVEBUFF2, ENVSETUP2, SPNOOP,
    SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP, SPNOOP
};*/

void (*ABI2[0x20])() = {SPNOOP, ADPCM2, CLEARBUFF2, UNKNOWN, ADDMIXER, RESAMPLE2, UNKNOWN, SEGMENT2, SETBUFF2,
    DUPLICATE2, DMEMMOVE2, LOADADPCM2, MIXER2, INTERLEAVE2, HILOGAIN, SETLOOP2, SPNOOP, INTERL2, ENVSETUP1, ENVMIXER2,
    LOADBUFF2, SAVEBUFF2, ENVSETUP2, SPNOOP, HILOGAIN, SPNOOP, DUPLICATE2, UNKNOWN, SPNOOP, SPNOOP, SPNOOP, SPNOOP};
/*
void (*ABI2[0x20])() = {
    SPNOOP , ADPCM2, CLEARBUFF2, SPNOOP, SPNOOP, RESAMPLE2  , SPNOOP  , SEGMENT2,
    SETBUFF2 , DUPLICATE2, DMEMMOVE2, LOADADPCM2, MIXER2, INTERLEAVE2, SPNOOP, SETLOOP2,
    SPNOOP, INTERL2 , ENVSETUP1, ENVMIXER2, LOADBUFF2, SAVEBUFF2, ENVSETUP2, SPNOOP,
    SPNOOP , SPNOOP, SPNOOP , SPNOOP    , SPNOOP  , SPNOOP    , SPNOOP  , SPNOOP
};
*/
/* NOTES:

  FILTER/SEGMENT - Still needs to be finished up... add FILTER?
  UNKNOWWN #27	 - Is this worth doing?  Looks like a pain in the ass just for WaveRace64
*/
