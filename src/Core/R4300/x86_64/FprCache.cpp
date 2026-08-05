/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/FprCache.hpp>

namespace
{
struct Slot
{
    int32_t fgr;
    bool dirty;
    bool locked;
    uint32_t stamp;
};

Slot g_slots[FPR_CACHE_SLOTS] = {
    {-1, false, false, 0},
    {-1, false, false, 0},
    {-1, false, false, 0},
};
static_assert(FPR_CACHE_SLOTS == 3, "keep g_slots' initialiser in sync with FPR_CACHE_SLOTS");

uint32_t g_clock;

bool g_double;

int32_t slot_xmm(int32_t slot)
{
    return FPR_CACHE_XMM_BASE + slot;
}

void *fpr_ptr_addr(int32_t fgr)
{
    return g_double ? (void *)&reg_cop1_double[fgr] : (void *)&reg_cop1_simple[fgr];
}

void emit_fill(int32_t slot)
{
    mov_r11_pm64(fpr_ptr_addr(g_slots[slot].fgr));
    if (g_double)
        movsd_xmm_pr11(slot_xmm(slot));
    else
        movss_xmm_pr11(slot_xmm(slot));
}

void emit_spill(int32_t slot)
{
    mov_r11_pm64(fpr_ptr_addr(g_slots[slot].fgr));
    if (g_double)
        movsd_pr11_xmm(slot_xmm(slot));
    else
        movss_pr11_xmm(slot_xmm(slot));
}

void drop(int32_t slot)
{
    if (g_slots[slot].fgr >= 0 && g_slots[slot].dirty) emit_spill(slot);
    g_slots[slot].fgr = -1;
    g_slots[slot].dirty = false;
    g_slots[slot].locked = false;
}

int32_t find(int32_t fgr)
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
        if (g_slots[i].fgr == fgr) return i;
    return -1;
}

bool is_cache_aware(uint32_t src)
{
    if (((src >> 26) & 0x3F) != 17) return false;

    const uint32_t fmt = (src >> 21) & 0x1F;
    if (fmt != 16 && fmt != 17) return false;

    const uint32_t funct = src & 0x3F;
    if (funct <= 7) return true;
    if (funct >= 0x30) return true;
    return false;
}

int32_t acquire()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
        if (g_slots[i].fgr < 0) return i;

    int32_t victim = -1;
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
    {
        if (g_slots[i].locked) continue;
        if (victim < 0 || g_slots[i].stamp < g_slots[victim].stamp) victim = i;
    }

    if (victim < 0)
    {
        g_core->log_error("[Dynarec] FATAL: FPR cache has no unlocked slot to evict");
        abort();
    }

    drop(victim);
    return victim;
}
}

void fpr_cache_reset()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
    {
        g_slots[i].fgr = -1;
        g_slots[i].dirty = false;
        g_slots[i].locked = false;
        g_slots[i].stamp = 0;
    }
    g_clock = 0;
}

void fpr_cache_begin(bool dbl)
{
#if FPR_CACHE_PERSIST
    if (dbl != g_double) fpr_cache_flush();
#else
    if (!fpr_cache_empty())
    {
        g_core->log_error("[Dynarec] FATAL: FPR cache not empty on entry to a COP1 instruction");
        abort();
    }
    fpr_cache_reset();
#endif
    g_double = dbl;
    fpr_cache_unlock_all();
}

void fpr_cache_end()
{
#if !FPR_CACHE_PERSIST
    fpr_cache_flush();
#endif
}

void fpr_cache_gate(uint32_t src)
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++) dst->reg_cache_infos.needed_xmm[i] = nullptr;

    if (!is_cache_aware(src)) fpr_cache_flush();
}

void fpr_cache_mark_live()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
    {
        dst->reg_cache_infos.needed_xmm[i] = g_slots[i].fgr >= 0 ? fpr_ptr_addr(g_slots[i].fgr) : nullptr;
        dst->reg_cache_infos.needed_xmm_double[i] = (unsigned char)g_double;
    }
}

void fpr_cache_reload_live()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
        if (g_slots[i].fgr >= 0) emit_fill(i);
}

bool fpr_cache_empty()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
        if (g_slots[i].fgr >= 0) return false;
    return true;
}

void fpr_cache_flush()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++) drop(i);
}

int32_t fpr_cache_read(int32_t fgr)
{
    int32_t slot = find(fgr);
    if (slot < 0)
    {
        slot = acquire();
        g_slots[slot].fgr = fgr;
        g_slots[slot].dirty = false;
        emit_fill(slot);
    }

    g_slots[slot].stamp = ++g_clock;
    return slot_xmm(slot);
}

int32_t fpr_cache_write(int32_t fgr)
{
    int32_t slot = find(fgr);
    if (slot < 0)
    {
        slot = acquire();
        g_slots[slot].fgr = fgr;
    }

    g_slots[slot].dirty = true;
    g_slots[slot].stamp = ++g_clock;
    return slot_xmm(slot);
}

void fpr_cache_lock(int32_t xmm)
{
    const int32_t slot = xmm - FPR_CACHE_XMM_BASE;
    if (slot >= 0 && slot < FPR_CACHE_SLOTS) g_slots[slot].locked = true;
}

void fpr_cache_unlock_all()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++) g_slots[i].locked = false;
}

void fpr_cache_spill_live()
{
    for (int32_t i = 0; i < FPR_CACHE_SLOTS; i++)
        if (g_slots[i].fgr >= 0 && g_slots[i].dirty) emit_spill(i);
}
