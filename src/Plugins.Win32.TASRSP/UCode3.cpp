/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.h"
#include "HLE.h"

static void SPNOOP()
{
    MessageBox(nullptr, std::format(L"Unknown/Unimplemented Audio Command {} in ABI 3", inst1 >> 24).c_str(),
               L"Audio HLE Error", MB_OK);
}

extern uint16_t ResampleLUT[0x200];

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

extern short hleMixerWorkArea[256];
extern uint16_t adpcmtable[0x88];

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

    short *inp = (short *)(BufferSpace + 0x4F0);
    short *out = (short *)(BufferSpace + 0x9D0);
    short *aux1 = (short *)(BufferSpace + 0xB40);
    short *aux2 = (short *)(BufferSpace + 0xCB0);
    short *aux3 = (short *)(BufferSpace + 0xE20);
    int32_t MainR;
    int32_t MainL;
    int32_t AuxR;
    int32_t AuxL;
    int i1, o1, a1, a2, a3;
    WORD AuxIncRate = 1;
    short zero[8];
    memset(zero, 0, 16);

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
        memcpy((uint8_t *)hleMixerWorkArea, rsp.rdram + addy, 80);
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

    for (int y = 0; y < (0x170 / 2); y++)
    {
        // Left
        LAcc += LAdder;
        LVol += (LAcc >> 16);
        LAcc &= 0xFFFF;

        // Right
        RAcc += RAdder;
        RVol += (RAcc >> 16);
        RAcc &= 0xFFFF;
        // ****************************************************************
        // Clamp Left
        if (LSig >= 0)
        {
            // VLT
            if (LVol > LTrg)
            {
                LVol = LTrg;
            }
        }
        else
        {
            // VGE
            if (LVol < LTrg)
            {
                LVol = LTrg;
            }
        }

        // Clamp Right
        if (RSig >= 0)
        {
            // VLT
            if (RVol > RTrg)
            {
                RVol = RTrg;
            }
        }
        else
        {
            // VGE
            if (RVol < RTrg)
            {
                RVol = RTrg;
            }
        }
        // ****************************************************************
        MainL = ((Dry * LVol) + 0x4000) >> 15;
        MainR = ((Dry * RVol) + 0x4000) >> 15;

        o1 = out[y ^ 1];
        a1 = aux1[y ^ 1];
        i1 = inp[y ^ 1];

        o1 += ((i1 * MainL) + 0x4000) >> 15;
        a1 += ((i1 * MainR) + 0x4000) >> 15;

        // ****************************************************************

        if (o1 > 32767)
            o1 = 32767;
        else if (o1 < -32768)
            o1 = -32768;

        if (a1 > 32767)
            a1 = 32767;
        else if (a1 < -32768)
            a1 = -32768;

        // ****************************************************************

        out[y ^ 1] = o1;
        aux1[y ^ 1] = a1;

        // ****************************************************************
        // if (!(flags&A_AUX)) {
        a2 = aux2[y ^ 1];
        a3 = aux3[y ^ 1];

        AuxL = ((Wet * LVol) + 0x4000) >> 15;
        AuxR = ((Wet * RVol) + 0x4000) >> 15;

        a2 += ((i1 * AuxL) + 0x4000) >> 15;
        a3 += ((i1 * AuxR) + 0x4000) >> 15;

        if (a2 > 32767)
            a2 = 32767;
        else if (a2 < -32768)
            a2 = -32768;

        if (a3 > 32767)
            a3 = 32767;
        else if (a3 < -32768)
            a3 = -32768;

        aux2[y ^ 1] = a2;
        aux3[y ^ 1] = a3;
    }
    //}

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
    memcpy(rsp.rdram + addy, (uint8_t *)hleMixerWorkArea, 80);
}

//*/
static void ENVMIXER3o()
{
    uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
    uint32_t addy = (inst2 & 0xFFFFFF); // + SEGMENTS[(inst2>>24)&0xf];

    //  ********* Make sure these conditions are met... ***********
    if ((AudioInBuffer | AudioOutBuffer | AudioAuxA | AudioAuxC | AudioAuxE | AudioCount) & 0x3)
    {
        MessageBox(NULL,
                   L"Unaligned EnvMixer... please report this to Azimer with the following information: RomTitle, "
                   L"Place in the rom it occurred, and any save state just before the error",
                   L"AudioHLE Error", MB_OK);
    }

    short *inp = (short *)(BufferSpace + 0x4F0);
    short *out = (short *)(BufferSpace + 0x9D0);
    short *aux1 = (short *)(BufferSpace + 0xB40);
    short *aux2 = (short *)(BufferSpace + 0xCB0);
    short *aux3 = (short *)(BufferSpace + 0xE20);

    int MainR;
    int MainL;
    int AuxR;
    int AuxL;
    int i1, o1, a1, a2, a3;
    WORD AuxIncRate = 1;
    short zero[8];
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
        memcpy((uint8_t *)hleMixerWorkArea, (rsp.rdram + addy), 80);
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

    for (int x = 0; x < (0x170 / 2); x++)
    {
        i1 = (int)inp[x ^ 1];
        o1 = (int)out[x ^ 1];
        a1 = (int)aux1[x ^ 1];
        if (AuxIncRate)
        {
            a2 = (int)aux2[x ^ 1];
            a3 = (int)aux3[x ^ 1];
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
    memcpy(rsp.rdram + addy, (uint8_t *)hleMixerWorkArea, 80);
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

    for (int x = 0; x < 0x170; x += 2)
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
    memcpy(BufferSpace + src, rsp.rdram + v0, cnt);
}

static void SAVEBUFF3()
{
    uint32_t v0;
    uint32_t cnt = (((inst1 >> 0xC) + 3) & 0xFFC);
    v0 = (inst2 & 0xfffffc);
    uint32_t src = (inst1 & 0xffc) + 0x4f0;
    memcpy(rsp.rdram + v0, BufferSpace + src, cnt);
}

static void LOADADPCM3()
{
    // Loads an ADPCM table - Works 100% Now 03-13-01
    uint32_t v0;
    v0 = (inst2 & 0xffffff);
    // memcpy (dmem+0x3f0, rsp.rdram+v0, inst1&0xffff);
    // assert ((inst1&0xffff) <= 0x80);
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
    // Verified to be 100% Accurate...
    BYTE Flags = (uint8_t)(inst2 >> 0x1c) & 0xff;
    // WORD Gain=(uint16_t)(inst1&0xffff);
    DWORD Address = (inst1 & 0xffffff); // + SEGMENTS[(inst2>>24)&0xf];
    WORD inPtr = (inst2 >> 12) & 0xf;
    // short *out=(int16_t *)(testbuff+(AudioOutBuffer>>2));
    short *out = (short *)(BufferSpace + (inst2 & 0xfff) + 0x4f0);
    BYTE *in = (BYTE *)(BufferSpace + ((inst2 >> 12) & 0xf) + 0x4f0);
    short count = (short)((inst2 >> 16) & 0xfff);
    BYTE icode;
    BYTE code;
    int vscale;
    WORD index;
    WORD j;
    int a[8];
    short *book1, *book2;

    memset(out, 0, 32);

    if (!(Flags & 0x1))
    {
        if (Flags & 0x2)
        {
            memcpy(out, &rsp.rdram[loopval], 32);
        }
        else
        {
            memcpy(out, &rsp.rdram[Address], 32);
        }
    }

    int l1 = out[15];
    int l2 = out[14];
    int inp1[8];
    int inp2[8];
    out += 16;
    while (count > 0)
    {
        // the first interation through, these values are
        // either 0 in the case of A_INIT, from a special
        // area of memory in the case of A_LOOP or just
        // the values we calculated the last time

        code = BufferSpace[(0x4f0 + inPtr) ^ 3];
        index = code & 0xf;
        index <<= 4; // index into the adpcm code table
        book1 = (short *)&adpcmtable[index];
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
        // which yields 8 short pcm values
        {
            icode = BufferSpace[(0x4f0 + inPtr) ^ 3];
            inPtr++;

            inp1[j] = (int16_t)((icode & 0xf0) << 8); // this will in effect be signed
            if (code < 12)
                inp1[j] = ((int)((int)inp1[j] * (int)vscale) >> 16);
            else
                int catchme = 1;
            j++;

            inp1[j] = (int16_t)((icode & 0xf) << 12);
            if (code < 12)
                inp1[j] = ((int)((int)inp1[j] * (int)vscale) >> 16);
            else
                int catchme = 1;
            j++;
        }
        j = 0;
        while (j < 8)
        {
            icode = BufferSpace[(0x4f0 + inPtr) ^ 3];
            inPtr++;

            inp2[j] = (short)((icode & 0xf0) << 8); // this will in effect be signed
            if (code < 12)
                inp2[j] = ((int)((int)inp2[j] * (int)vscale) >> 16);
            else
                int catchme = 1;
            j++;

            inp2[j] = (short)((icode & 0xf) << 12);
            if (code < 12)
                inp2[j] = ((int)((int)inp2[j] * (int)vscale) >> 16);
            else
                int catchme = 1;
            j++;
        }

        a[0] = (int)book1[0] * (int)l1;
        a[0] += (int)book2[0] * (int)l2;
        a[0] += (int)inp1[0] * (int)2048;

        a[1] = (int)book1[1] * (int)l1;
        a[1] += (int)book2[1] * (int)l2;
        a[1] += (int)book2[0] * inp1[0];
        a[1] += (int)inp1[1] * (int)2048;

        a[2] = (int)book1[2] * (int)l1;
        a[2] += (int)book2[2] * (int)l2;
        a[2] += (int)book2[1] * inp1[0];
        a[2] += (int)book2[0] * inp1[1];
        a[2] += (int)inp1[2] * (int)2048;

        a[3] = (int)book1[3] * (int)l1;
        a[3] += (int)book2[3] * (int)l2;
        a[3] += (int)book2[2] * inp1[0];
        a[3] += (int)book2[1] * inp1[1];
        a[3] += (int)book2[0] * inp1[2];
        a[3] += (int)inp1[3] * (int)2048;

        a[4] = (int)book1[4] * (int)l1;
        a[4] += (int)book2[4] * (int)l2;
        a[4] += (int)book2[3] * inp1[0];
        a[4] += (int)book2[2] * inp1[1];
        a[4] += (int)book2[1] * inp1[2];
        a[4] += (int)book2[0] * inp1[3];
        a[4] += (int)inp1[4] * (int)2048;

        a[5] = (int)book1[5] * (int)l1;
        a[5] += (int)book2[5] * (int)l2;
        a[5] += (int)book2[4] * inp1[0];
        a[5] += (int)book2[3] * inp1[1];
        a[5] += (int)book2[2] * inp1[2];
        a[5] += (int)book2[1] * inp1[3];
        a[5] += (int)book2[0] * inp1[4];
        a[5] += (int)inp1[5] * (int)2048;

        a[6] = (int)book1[6] * (int)l1;
        a[6] += (int)book2[6] * (int)l2;
        a[6] += (int)book2[5] * inp1[0];
        a[6] += (int)book2[4] * inp1[1];
        a[6] += (int)book2[3] * inp1[2];
        a[6] += (int)book2[2] * inp1[3];
        a[6] += (int)book2[1] * inp1[4];
        a[6] += (int)book2[0] * inp1[5];
        a[6] += (int)inp1[6] * (int)2048;

        a[7] = (int)book1[7] * (int)l1;
        a[7] += (int)book2[7] * (int)l2;
        a[7] += (int)book2[6] * inp1[0];
        a[7] += (int)book2[5] * inp1[1];
        a[7] += (int)book2[4] * inp1[2];
        a[7] += (int)book2[3] * inp1[3];
        a[7] += (int)book2[2] * inp1[4];
        a[7] += (int)book2[1] * inp1[5];
        a[7] += (int)book2[0] * inp1[6];
        a[7] += (int)inp1[7] * (int)2048;

        for (j = 0; j < 8; j++)
        {
            a[j ^ 1] >>= 11;
            if (a[j ^ 1] > 32767)
                a[j ^ 1] = 32767;
            else if (a[j ^ 1] < -32768)
                a[j ^ 1] = -32768;
            *(out++) = a[j ^ 1];
            //*(out+j)=a[j^1];
        }
        // out += 0x10;
        l1 = a[6];
        l2 = a[7];

        a[0] = (int)book1[0] * (int)l1;
        a[0] += (int)book2[0] * (int)l2;
        a[0] += (int)inp2[0] * (int)2048;

        a[1] = (int)book1[1] * (int)l1;
        a[1] += (int)book2[1] * (int)l2;
        a[1] += (int)book2[0] * inp2[0];
        a[1] += (int)inp2[1] * (int)2048;

        a[2] = (int)book1[2] * (int)l1;
        a[2] += (int)book2[2] * (int)l2;
        a[2] += (int)book2[1] * inp2[0];
        a[2] += (int)book2[0] * inp2[1];
        a[2] += (int)inp2[2] * (int)2048;

        a[3] = (int)book1[3] * (int)l1;
        a[3] += (int)book2[3] * (int)l2;
        a[3] += (int)book2[2] * inp2[0];
        a[3] += (int)book2[1] * inp2[1];
        a[3] += (int)book2[0] * inp2[2];
        a[3] += (int)inp2[3] * (int)2048;

        a[4] = (int)book1[4] * (int)l1;
        a[4] += (int)book2[4] * (int)l2;
        a[4] += (int)book2[3] * inp2[0];
        a[4] += (int)book2[2] * inp2[1];
        a[4] += (int)book2[1] * inp2[2];
        a[4] += (int)book2[0] * inp2[3];
        a[4] += (int)inp2[4] * (int)2048;

        a[5] = (int)book1[5] * (int)l1;
        a[5] += (int)book2[5] * (int)l2;
        a[5] += (int)book2[4] * inp2[0];
        a[5] += (int)book2[3] * inp2[1];
        a[5] += (int)book2[2] * inp2[2];
        a[5] += (int)book2[1] * inp2[3];
        a[5] += (int)book2[0] * inp2[4];
        a[5] += (int)inp2[5] * (int)2048;

        a[6] = (int)book1[6] * (int)l1;
        a[6] += (int)book2[6] * (int)l2;
        a[6] += (int)book2[5] * inp2[0];
        a[6] += (int)book2[4] * inp2[1];
        a[6] += (int)book2[3] * inp2[2];
        a[6] += (int)book2[2] * inp2[3];
        a[6] += (int)book2[1] * inp2[4];
        a[6] += (int)book2[0] * inp2[5];
        a[6] += (int)inp2[6] * (int)2048;

        a[7] = (int)book1[7] * (int)l1;
        a[7] += (int)book2[7] * (int)l2;
        a[7] += (int)book2[6] * inp2[0];
        a[7] += (int)book2[5] * inp2[1];
        a[7] += (int)book2[4] * inp2[2];
        a[7] += (int)book2[3] * inp2[3];
        a[7] += (int)book2[2] * inp2[4];
        a[7] += (int)book2[1] * inp2[5];
        a[7] += (int)book2[0] * inp2[6];
        a[7] += (int)inp2[7] * (int)2048;

        for (j = 0; j < 8; j++)
        {
            a[j ^ 1] >>= 11;
            if (a[j ^ 1] > 32767)
                a[j ^ 1] = 32767;
            else if (a[j ^ 1] < -32768)
                a[j ^ 1] = -32768;
            *(out++) = a[j ^ 1];
            //*(out+j+0x1f8)=a[j^1];
        }
        l1 = a[6];
        l2 = a[7];

        count -= 32;
    }
    out -= 16;
    memcpy(&rsp.rdram[Address], out, 32);
}

static void RESAMPLE3()
{
    BYTE Flags = (uint8_t)((inst2 >> 0x1e));
    DWORD Pitch = ((inst2 >> 0xe) & 0xffff) << 1;
    uint32_t addy = (inst1 & 0xffffff);
    DWORD Accum = 0;
    DWORD location;
    int16_t *lut;
    short *dst;
    int16_t *src;
    dst = (short *)(BufferSpace);
    src = (int16_t *)(BufferSpace);
    uint32_t srcPtr = ((((inst2 >> 2) & 0xfff) + 0x4f0) / 2);
    uint32_t dstPtr; //=(AudioOutBuffer/2);
    int32_t temp;
    int32_t accum;

    // if (addy > (1024*1024*8))
    //	addy = (inst2 & 0xffffff);

    srcPtr -= 4;

    if (inst2 & 0x3)
    {
        dstPtr = 0x660 / 2;
    }
    else
    {
        dstPtr = 0x4f0 / 2;
    }

    if ((Flags & 0x1) == 0)
    {
        for (int x = 0; x < 4; x++) // memcpy (src+srcPtr, rsp.rdram+addy, 0x8);
            src[(srcPtr + x) ^ 1] = ((uint16_t *)rsp.rdram)[((addy / 2) + x) ^ 1];
        Accum = *(uint16_t *)(rsp.rdram + addy + 10);
    }
    else
    {
        for (int x = 0; x < 4; x++) src[(srcPtr + x) ^ 1] = 0; //*(uint16_t *)(rsp.rdram+((addy+x)^2));
    }

    // if ((Flags & 0x2))
    //	__asm int 3;

    for (int i = 0; i < 0x170 / 2; i++)
    {
        location = (((Accum * 0x40) >> 0x10) * 8);
        // location = (Accum >> 0xa) << 0x3;
        lut = (int16_t *)(((uint8_t *)ResampleLUT) + location);

        temp = ((int32_t)*(int16_t *)(src + ((srcPtr + 0) ^ 1)) * ((int32_t)((int16_t)lut[0])));
        accum = (int32_t)(temp >> 15);

        temp = ((int32_t)*(int16_t *)(src + ((srcPtr + 1) ^ 1)) * ((int32_t)((int16_t)lut[1])));
        accum += (int32_t)(temp >> 15);

        temp = ((int32_t)*(int16_t *)(src + ((srcPtr + 2) ^ 1)) * ((int32_t)((int16_t)lut[2])));
        accum += (int32_t)(temp >> 15);

        temp = ((int32_t)*(int16_t *)(src + ((srcPtr + 3) ^ 1)) * ((int32_t)((int16_t)lut[3])));
        accum += (int32_t)(temp >> 15);
        /*		temp =  ((int64_t)*(int16_t*)(src+((srcPtr+0)^1))*((int64_t)((int16_t)lut[0]<<1)));
                if (temp & 0x8000) temp = (temp^0x8000) + 0x10000;
                else temp = (temp^0x8000);
                temp = (int32_t)(temp >> 16);
                if ((int32_t)temp > 32767) temp = 32767;
                if ((int32_t)temp < -32768) temp = -32768;
                accum = (int32_t)(int16_t)temp;

                temp = ((int64_t)*(int16_t*)(src+((srcPtr+1)^1))*((int64_t)((int16_t)lut[1]<<1)));
                if (temp & 0x8000) temp = (temp^0x8000) + 0x10000;
                else temp = (temp^0x8000);
                temp = (int32_t)(temp >> 16);
                if ((int32_t)temp > 32767) temp = 32767;
                if ((int32_t)temp < -32768) temp = -32768;
                accum += (int32_t)(int16_t)temp;

                temp = ((int64_t)*(int16_t*)(src+((srcPtr+2)^1))*((int64_t)((int16_t)lut[2]<<1)));
                if (temp & 0x8000) temp = (temp^0x8000) + 0x10000;
                else temp = (temp^0x8000);
                temp = (int32_t)(temp >> 16);
                if ((int32_t)temp > 32767) temp = 32767;
                if ((int32_t)temp < -32768) temp = -32768;
                accum += (int32_t)(int16_t)temp;

                temp = ((int64_t)*(int16_t*)(src+((srcPtr+3)^1))*((int64_t)((int16_t)lut[3]<<1)));
                if (temp & 0x8000) temp = (temp^0x8000) + 0x10000;
                else temp = (temp^0x8000);
                temp = (int32_t)(temp >> 16);
                if ((int32_t)temp > 32767) temp = 32767;
                if ((int32_t)temp < -32768) temp = -32768;
                accum += (int32_t)(int16_t)temp;*/

        if (accum > 32767) accum = 32767;
        if (accum < -32768) accum = -32768;

        dst[dstPtr ^ 1] = (accum);
        dstPtr++;
        Accum += Pitch;
        srcPtr += (Accum >> 16);
        Accum &= 0xffff;
    }
    for (int x = 0; x < 4; x++) ((uint16_t *)rsp.rdram)[((addy / 2) + x) ^ 1] = src[(srcPtr + x) ^ 1];
    *(uint16_t *)(rsp.rdram + addy + 10) = Accum;
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

    for (int x = 0; x < (0x170 / 4); x++)
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
    BYTE sync;

    BYTE error_protection	: 1;	//  0=yes, 1=no
    BYTE lay				: 2;	// 4-lay = layerI, II or III
    BYTE version			: 1;	// 3=mpeg 1.0, 2=mpeg 2.5 0=mpeg 2.0
    BYTE sync2				: 4;

    BYTE extension			: 1;    // Unknown
    BYTE padding			: 1;    // padding
    BYTE sampling_freq		: 2;	// see table below
    BYTE bitrate_index		: 4;	//     see table below

    BYTE emphasis			: 2;	//see table below
    BYTE original			: 1;	// 0=no 1=yes
    BYTE copyright			: 1;	// 0=no 1=yes
    BYTE mode_ext			: 2;    // used with "joint stereo" mode
    BYTE mode				: 2;    // Channel Mode
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
    //__asm int 3;
    // fprintf (fp, "mp3addy: inst1: %08X, inst2: %08X, loopval: %08X\n", inst1, inst2, loopval);
}

extern "C"
{
    void rsp_run();
    void mp3setup(DWORD inst1, DWORD inst2, DWORD t8);
}

extern uint32_t base, dmembase;

extern "C"
{
    extern char *pDMEM;
}

void MP3();
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
    //__asm int 3;
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
    // MessageBox (NULL, "Help", "ABI 3 Command 0", MB_OK);
    // ChangeABI (5);
}

void (*ABI3[0x20])() = {DISABLE, ADPCM3,  CLEARBUFF3, ENVMIXER3,  LOADBUFF3, RESAMPLE3,   SAVEBUFF3,  MP3,
                        MP3ADDY, SETVOL3, DMEMMOVE3,  LOADADPCM3, MIXER3,    INTERLEAVE3, WHATISTHIS, SETLOOP3,
                        SPNOOP,  SPNOOP,  SPNOOP,     SPNOOP,     SPNOOP,    SPNOOP,      SPNOOP,     SPNOOP,
                        SPNOOP,  SPNOOP,  SPNOOP,     SPNOOP,     SPNOOP,    SPNOOP,      SPNOOP,     SPNOOP};
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
