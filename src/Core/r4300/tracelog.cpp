/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "tracelog.h"
#include "disasm.h"
#include "r4300.h"

bool enabled = false;
bool use_binary = false;

FILE* log_file;
char traceLoggingBuf[0x10000];
char* traceLoggingPointer = traceLoggingBuf;

bool core_vr_is_tracelog_active()
{
    return enabled;
}

void flush_buf()
{
    fwrite(traceLoggingBuf, 1, traceLoggingPointer - traceLoggingBuf, log_file);
    traceLoggingPointer = traceLoggingBuf;
}

void write_buf()
{
    const char* const buflength = traceLoggingBuf + sizeof(traceLoggingBuf) -
    512;
    if (traceLoggingPointer >= buflength)
    {
        flush_buf();
    }
}


void log_bin(uint32_t pc, uint32_t w)
{
    char*& p = traceLoggingPointer;
    INSTDECODE decode;
    // little endian
#define HEX8(n)        \
    *(uint32_t*)p = n; \
    p += 4

    DecodeInstruction(w, &decode);
    HEX8(pc);
    HEX8(w);
    INSTOPERAND& o = decode.operand;
    // n�͌ォ��f�R�[�h����΂킩��
#define REGCPU(n) \
    HEX8(reg[n])
#define REGCPU2(n, m) \
    REGCPU(n);        \
    REGCPU(m);
    // 10�i��
#define REGFPU(n) \
    HEX8(*(uint32_t*)reg_cop1_simple[n])
#define REGFPU2(n, m) \
    REGFPU(n);        \
    REGFPU(m)
#define NONE           \
    *(uint32_t*)p = 0; \
    p += 4
#define NONE2 \
    NONE;     \
    NONE

    switch (decode.format)
    {
    case INSTF_NONE:
        NONE2;
        break;
    case INSTF_J:
    case INSTF_0BRANCH:
        NONE2;
        break;
    case INSTF_LUI:
        NONE2;
        break;
    case INSTF_1BRANCH:
    case INSTF_JR:
    case INSTF_ISIGN:
    case INSTF_IUNSIGN:
        REGCPU(o.i.rs);
        NONE;
        break;
    case INSTF_2BRANCH:
        REGCPU2(o.i.rs, o.i.rt);
        break;
    case INSTF_ADDRW:
        HEX8(reg[o.i.rs] + (int16_t)o.i.immediate);
        REGCPU(o.i.rt);
        break;
    case INSTF_ADDRR:
        HEX8(reg[o.i.rs] + (int16_t)o.i.immediate);
        NONE;
        break;
    case INSTF_LFW:
        HEX8(reg[o.lf.base] + (int16_t)o.lf.offset);
        REGFPU(o.lf.ft);
        break;
    case INSTF_LFR:
        HEX8(reg[o.lf.base] + (int16_t)o.lf.offset);
        NONE;
        break;
    case INSTF_R1:
        REGCPU(o.r.rd);
        NONE;
        break;
    case INSTF_R2:
        REGCPU2(o.i.rs, o.i.rt);
        break;
    case INSTF_R3:
        REGCPU2(o.i.rs, o.i.rt);
        break;
    case INSTF_MTC0:
    case INSTF_MTC1:
    case INSTF_SA:
        REGCPU(o.r.rt);
        NONE;
        break;
    case INSTF_R2F:
        REGFPU(o.cf.fs);
        NONE;
        break;
    case INSTF_R3F:
    case INSTF_C:
        REGFPU2(o.cf.fs, o.cf.ft);
        break;
    case INSTF_MFC0:
        NONE2;
        break;
    case INSTF_MFC1:
        REGFPU(((uint8_t)o.r.rs));
        NONE;
        break;
    }
    write_buf();
#undef HEX8
#undef REGCPU
#undef REGFPU
#undef REGCPU2
#undef REGFPU2
}

void log(uint32_t pc, uint32_t w)
{
    char*& p = traceLoggingPointer;
    INSTDECODE decode;
    const char* const x = "0123456789abcdef";
#define HEX8(n)                          \
    p[0] = x[(uint32_t)(n) >> 28 & 0xF]; \
    p[1] = x[(uint32_t)(n) >> 24 & 0xF]; \
    p[2] = x[(uint32_t)(n) >> 20 & 0xF]; \
    p[3] = x[(uint32_t)(n) >> 16 & 0xF]; \
    p[4] = x[(uint32_t)(n) >> 12 & 0xF]; \
    p[5] = x[(uint32_t)(n) >> 8 & 0xF];  \
    p[6] = x[(uint32_t)(n) >> 4 & 0xF];  \
    p[7] = x[(uint32_t)(n) & 0xF];       \
    p += 8;

    DecodeInstruction(w, &decode);
    HEX8(pc);
    *(p++) = ':';
    *(p++) = ' ';
    HEX8(w);
    *(p++) = ' ';
    const char* ps = p;
    if (w == 0x00000000)
    {
        *(p++) = 'n';
        *(p++) = 'o';
        *(p++) = 'p';
    }
    else
    {
        for (const char* q = GetOpecodeString(&decode); *q; q++)
        {
            *(p++) = *q;
        }
        *(p++) = ' ';
        p = GetOperandString(p, &decode, pc);
    }
    for (int32_t i = p - ps + 3; i < 24; i += 4)
    {
        *(p++) = '\t';
    }
    *(p++) = ';';
    INSTOPERAND& o = decode.operand;
#define REGCPU(n)                                         \
    if ((n) != 0)                                         \
    {                                                     \
        for (const char* l = CPURegisterName[n]; *l; l++) \
        {                                                 \
            *(p++) = *l;                                  \
        }                                                 \
        *(p++) = '=';                                     \
        HEX8(reg[n]);                                     \
    }
#define REGCPU2(n, m)           \
    REGCPU(n);                  \
    if ((n) != (m) && (m) != 0) \
    {                           \
        C;                      \
        REGCPU(m);              \
    }
    // 10�i��
#define REGFPU(n)         \
    *(p++) = 'f';         \
    *(p++) = x[(n) / 10]; \
    *(p++) = x[(n) % 10]; \
    *(p++) = '=';         \
    p += sprintf(p, "%f", *reg_cop1_simple[n])
#define REGFPU2(n, m) \
    REGFPU(n);        \
    if ((n) != (m))   \
    {                 \
        C;            \
        REGFPU(m);    \
    }
#define C *(p++) = ','

    if (delay_slot)
    {
        *(p++) = '#';
    }
    switch (decode.format)
    {
    case INSTF_NONE:
        break;
    case INSTF_J:
    case INSTF_0BRANCH:
        break;
    case INSTF_LUI:
        break;
    case INSTF_1BRANCH:
    case INSTF_JR:
    case INSTF_ISIGN:
    case INSTF_IUNSIGN:
        REGCPU(o.i.rs);
        break;
    case INSTF_2BRANCH:
        REGCPU2(o.i.rs, o.i.rt);
        break;
    case INSTF_ADDRW:
        REGCPU(o.i.rt);
        if (o.i.rt != 0)
        {
            C;
        }
    case INSTF_ADDRR:
        *(p++) = '@';
        *(p++) = '=';
        HEX8(reg[o.i.rs] + (int16_t)o.i.immediate);
        break;
    case INSTF_LFW:
        REGFPU(o.lf.ft);
        C;
    case INSTF_LFR:
        *(p++) = '@';
        *(p++) = '=';
        HEX8(reg[o.lf.base] + (int16_t)o.lf.offset);
        break;
    case INSTF_R1:
        REGCPU(o.r.rd);
        break;
    case INSTF_R2:
        REGCPU2(o.i.rs, o.i.rt);
        break;
    case INSTF_R3:
        REGCPU2(o.i.rs, o.i.rt);
        break;
    case INSTF_MTC0:
    case INSTF_MTC1:
    case INSTF_SA:
        REGCPU(o.r.rt);
        break;
    case INSTF_R2F:
        REGFPU(o.cf.fs);
        break;
    case INSTF_R3F:
    case INSTF_C:
        REGFPU2(o.cf.fs, o.cf.ft);
        break;
    case INSTF_MFC0:
        break;
    case INSTF_MFC1:
        REGFPU(((uint8_t)o.r.rs));
        break;
    }
    *(p++) = '\n';

    write_buf();
#undef HEX8
#undef REGCPU
#undef REGFPU
#undef REGCPU2
#undef REGFPU2
#undef C
}

void tracelog_log_pure()
{
    if (!use_binary)
    {
        log(interp_addr, vr_op);
    }
    else
    {
        log_bin(interp_addr, vr_op);
    }
}

void tracelog_log_interp_ops()
{
    if (enabled)
    {
        if (!use_binary)
        {
            log(PC->addr, PC->src);
        }
        else
        {
            log_bin(PC->addr, PC->src);
        }
    }
    PC->s_ops();
}

void core_tl_start(std::filesystem::path path, bool binary, bool append)
{
    use_binary = binary;
    log_file = _wfopen(path.wstring().c_str(), L"wb");

    enabled = true;
    if (interpcore == 0)
    {
        core_vr_recompile(UINT32_MAX);
    }
}

void core_tl_stop()
{
    enabled = false;
    flush_buf();
    fclose(log_file);
}
