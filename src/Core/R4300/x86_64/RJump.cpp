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
#include <Alloc.hpp>
#include <utility>

// NOTE: dynarec isn't compatible with the game debugger

// dyna_jump redirects a return address at a small thunk rather than jumping itself. It is reached
// through a chain of call_reg64s that each `push r11`, so a direct jump would leave RSP 8 low;
// instead it points the outer call_reg64's return slot (*return_address) at a thunk the patched
// `ret` lands in, C chain unwound:  add rsp,8 (undo the push r11) ; mov r11,target ; jmp r11.
// need_map target = the instruction's jump_wrapper (expects this RSP, doesn't touch it).

// dyna_jump runs on every block transition (hottest path). Re-synthesizing the thunk each call
// is slow, so it is generated once per instruction, cached in reg_cache_infos.jump_thunk, reused.
// Safe: the thunk never bakes the target (need_map -> stable jump_wrapper address; non-map ->
// block->code + local_addr recomputed at runtime from stable field addresses). Only a need_map
// flip invalidates it (tracked by jump_thunk_need_map); identical bytes each time -> reentrancy-safe.

// Bump-allocated, never freed (referenced by patched return addresses / cached per instruction).
static constexpr size_t THUNK_MAX_SIZE = 64;         // largest thunk is 33 bytes
static constexpr size_t THUNK_POOL_SIZE = 64 * 1024; // VirtualAlloc reservation granularity

static unsigned char *thunk_alloc()
{
    static unsigned char *pool = nullptr;
    static size_t used = 0;

    if (!pool || used + THUNK_MAX_SIZE > THUNK_POOL_SIZE)
    {
        pool = (unsigned char *)malloc_exec(THUNK_POOL_SIZE);
        used = 0;
    }

    unsigned char *slot = pool + used;
    used += THUNK_MAX_SIZE;
    return slot;
}

// Emit the dyna_jump thunk for `cur` into `p` (two forms; see rationale above).
static void emit_jump_thunk(unsigned char *p, precomp_block *block, precomp_instr *cur, int32_t need_map)
{
    // add rsp, 8   (48 83 C4 08)  — undo the push r11 from the outer call_reg64
    *p++ = 0x48;
    *p++ = 0x83;
    *p++ = 0xC4;
    *p++ = 0x08;

    if (need_map)
    {
        // need_map: jump to the stable jump_wrapper address (wrapper doesn't touch RSP).
        uintptr_t target = (uintptr_t)(cur->reg_cache_infos.jump_wrapper);

        // mov r11, imm64(target)   (49 BB <8 bytes>)
        *p++ = 0x49;
        *p++ = 0xBB;
        for (int i = 0; i < 8; ++i) *p++ = (unsigned char)((target >> (i * 8)) & 0xFF);

        // jmp r11   (49 FF E3)
        *p++ = 0x49;
        *p++ = 0xFF;
        *p++ = 0xE3;
    }
    else
    {
        // non-map: recompute block->code + local_addr at runtime (baking would go stale when
        // realloc moves block->code / reassigns local_addr).
        uintptr_t p_code = (uintptr_t)&block->code;
        uintptr_t p_local = (uintptr_t)&cur->local_addr;

        // mov r10, imm64(&block->code)   (49 BA <8 bytes>)
        *p++ = 0x49;
        *p++ = 0xBA;
        for (int i = 0; i < 8; ++i) *p++ = (unsigned char)((p_code >> (i * 8)) & 0xFF);

        // mov r11, [r10]        (4D 8B 1A)   r11 = block->code (live)
        *p++ = 0x4D;
        *p++ = 0x8B;
        *p++ = 0x1A;

        // mov r10, imm64(&cur->local_addr)   (49 BA <8 bytes>)
        *p++ = 0x49;
        *p++ = 0xBA;
        for (int i = 0; i < 8; ++i) *p++ = (unsigned char)((p_local >> (i * 8)) & 0xFF);

        // add r11, [r10]        (4D 03 1A)   r11 += local_addr (live)
        *p++ = 0x4D;
        *p++ = 0x03;
        *p++ = 0x1A;

        // jmp r11   (49 FF E3)
        *p++ = 0x49;
        *p++ = 0xFF;
        *p++ = 0xE3;
    }
}

void dyna_jump()
{
    // PC may be STALE: an older compilation of this page, kept alive by the deferred_free_list,
    // whose ->local_addr/->need_map/->jump_wrapper describe the OLD layout. Pairing a stale
    // local_addr with the CURRENT block->code overruns into non-executable memory (DEP fault).
    // Re-derive the instr from the current block so code/local_addr/need_map/jump_wrapper are all
    // from the live compilation (mirrors jump_to_func). A freshly-invalidated block yields a
    // NOTCOMPILED stub (local_addr == 0), so we jump to block->code + 0 and recompilation kicks in.
    precomp_block *block = blocks[PC->addr >> 12];
    precomp_instr *cur;
    if (!block || !block->block)
    {
        // Page has no live block: nothing to re-derive from, and no newer compilation means PC
        // can't be stale here. Fall back to the pair x86 uses (see x86/RJump.cpp). Keep going
        // through the thunk below -- its `add rsp,8` is what balances call_reg64's `push r11`.
        block = actual;
        cur = PC;
    }
    else
    {
        cur = block->block + ((PC->addr - block->start) >> 2);
    }

    int32_t need_map = cur->reg_cache_infos.need_map;

    // Reuse the cached thunk; regenerate only on first use or a need_map flip.
    unsigned char *thunk = cur->reg_cache_infos.jump_thunk;
    if (!thunk || cur->reg_cache_infos.jump_thunk_need_map != need_map)
    {
        thunk = thunk_alloc();
        emit_jump_thunk(thunk, block, cur, need_map);
        cur->reg_cache_infos.jump_thunk = thunk;
        cur->reg_cache_infos.jump_thunk_need_map = need_map;
    }

    // Patch *return_address to jump to the thunk instead of falling through.
    *return_address = (uintptr_t)thunk;
}

// dyna_stop() jumps out of recompiled code back to dyna_start() over the intervening JIT/C frames.
// MSVC x64 longjmp can't be used: it unwinds via RtlUnwindEx, which walks SEH tables that
// recompiled code / thunks don't register -> faults. RtlRestoreContext restores registers without
// unwinding, which is what we want. Nothing is cleaned up, so dynarec callouts must not throw.
namespace
{
#ifdef _WIN32
CONTEXT g_dyna_ctx;
#else
jmp_buf g_dyna_ctx;
#endif

// RtlCaptureContext, unlike setjmp, gives no way to tell a resume apart from the
// initial call, so the distinction is carried in memory. Written before the context is
// restored, hence volatile.
volatile bool g_dyna_stopped;
} // namespace

// Saves the resume point. Like setjmp, this returns twice: false on the initial call and
// true once dyna_restore_context() jumps back to it. Must be a macro for the same reason
// setjmp is one — the save has to happen in the caller's own frame.
#ifdef _WIN32
#define dyna_save_context() (RtlCaptureContext(&g_dyna_ctx), (bool)g_dyna_stopped)
#else
#define dyna_save_context() (setjmp(g_dyna_ctx) != 0)
#endif

[[noreturn]] static void dyna_restore_context() noexcept
{
#ifdef _WIN32
    RtlRestoreContext(&g_dyna_ctx, nullptr);
    std::unreachable(); // RtlRestoreContext isn't declared noreturn, but never comes back
#else
    longjmp(g_dyna_ctx, 1);
#endif
}

// Entry trampoline: save the caller's r15 (callee-saved, keeps the ABI contract), set r15 = dynarec
// base (g_dynarec_base in Assemble.cpp), call the recompiled entry, restore r15. dyna_start is the
// only cold C->JIT entry; the rest come from already-running code. RSP stays aligned (push ->
// %16==0, call -> entry at %16==8). Normal exit is via dyna_stop, so pop/ret only run on a return.
static void (*dynarec_enter)(void (*code)()) = nullptr;

static void build_dynarec_enter()
{
    unsigned char *p = (unsigned char *)malloc_exec(32);
    dynarec_enter = (void (*)(void (*)()))p;

    // push r15   (41 57)  — save the caller's r15 (callee-saved)
    *p++ = 0x41;
    *p++ = 0x57;

    // mov r15, imm64(g_dynarec_base)   (49 BF <8 bytes>)
    *p++ = 0x49;
    *p++ = 0xBF;
    uintptr_t base = g_dynarec_base;
    for (int i = 0; i < 8; ++i) *p++ = (unsigned char)((base >> (i * 8)) & 0xFF);

        // call <arg register>  — the code pointer arrives in the first integer-arg register.
#ifdef _WIN32
    *p++ = 0xFF;
    *p++ = 0xD1; // call rcx (Win64 first arg)
#else
    *p++ = 0xFF;
    *p++ = 0xD7; // call rdi (System V first arg)
#endif

    // pop r15   (41 5F)  — restore the caller's r15
    *p++ = 0x41;
    *p++ = 0x5F;

    // ret   (C3)
    *p++ = 0xC3;
}

void dyna_start(void (*code)())
{
    core_executing = true;
    g_core->callbacks.core_executing_changed(core_executing);

    g_dyna_stopped = false;

    if (!dynarec_enter) build_dynarec_enter();

    if (!dyna_save_context())
    {
        dynarec_enter(code);
    }
}

void dyna_stop()
{
    g_dyna_stopped = true;
    dyna_restore_context();
}
