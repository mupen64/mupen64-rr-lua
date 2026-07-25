/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <Alloc.hpp>
#include <utility>

// NOTE: dynarec isn't compatible with the game debugger

// dyna_jump enters the next block by generating a small thunk and redirecting a return
// address at it, rather than jumping there itself.
//
// It is reached from recompiled code through a chain of call_reg64s, each of which pushes
// r11 in its prologue, so simply jumping to the target would leave RSP 8 bytes low. Instead
// dyna_jump writes a thunk, points the outer call_reg64's return-address slot
// (*return_address) at it, and returns normally. When that call_reg64's `ret` pops the
// patched slot, control lands in the thunk with the C chain already unwound:
//
//   add rsp, 8      undo the outer call_reg64's `push r11`, restoring its entry RSP
//   mov r11, target
//   jmp r11
//
// For need_map the target is the instruction's jump_wrapper, which expects exactly that
// entry RSP and does not touch RSP itself (see build_wrapper).

// Thunk slots come from one pre-allocated executable pool, handed out round-robin.
// Allocating per call instead (malloc_exec -> VirtualAlloc MEM_RESERVE|MEM_COMMIT) leaks a
// 64KB reservation every time and is never freed before shutdown; since dyna_jump runs for
// every register-cache-mapped jump, address space runs out, VirtualAlloc returns NULL and
// the first byte-store below faults on a null pointer.
//
// Reuse is safe because a thunk is consumed almost immediately: it runs as soon as the
// patched `ret` pops it, while the call chain is unwinding, and no other dyna_jump can run
// in that window. NUM_SLOTS is oversized purely as a guard against unexpected reentrancy.
static constexpr size_t THUNK_SLOT_SIZE = 64; // max thunk is 33 bytes
static constexpr size_t THUNK_NUM_SLOTS = 1024;

static unsigned char* jump_thunk_buf()
{
    static unsigned char* pool = nullptr;
    static size_t next_slot = 0;

    if (!pool)
        pool = (unsigned char*)malloc_exec(THUNK_SLOT_SIZE * THUNK_NUM_SLOTS);

    unsigned char* slot = pool + (next_slot * THUNK_SLOT_SIZE);
    next_slot = (next_slot + 1) % THUNK_NUM_SLOTS;
    return slot;
}

void dyna_jump()
{
    // PC may be a STALE pointer into a superseded precomp_instr array: an older
    // compilation of this page that was invalidated and recompiled into a fresh
    // block->block array + a new (smaller) code buffer. The old array is kept alive
    // by the deferred_free_list, so PC still dereferences — but its ->local_addr,
    // ->need_map and ->jump_wrapper describe the OLD compilation.
    //
    // The buffer we actually jump into is the CURRENT block's ->code
    // (blocks[PC->addr >> 12]->code). Pairing that with the stale PC->local_addr
    // overruns the new buffer (old local_addr > new code_length) and lands in
    // non-executable memory → DEP fault ("execute non-executable address").
    //
    // Re-derive the instruction from the CURRENT block for this PC->addr so that
    // code, local_addr, need_map and jump_wrapper all come from the same live
    // compilation (mirrors jump_to_func's `PC = actual->block + ((addr-start)>>2)`).
    // For a freshly-invalidated block this yields a NOTCOMPILED stub
    // (local_addr == 0), so we jump to block->code + 0 and recompilation kicks in.
    precomp_block* block = blocks[PC->addr >> 12];
    precomp_instr* cur = block->block + ((PC->addr - block->start) >> 2);

    bool need_map = cur->reg_cache_infos.need_map;

    unsigned char* p = jump_thunk_buf();
    unsigned char* thunk_addr = p;  // Save start address for patching return_address

    // add rsp, 8   (48 83 C4 08)  — undo the push r11 from call_reg64
    *p++ = 0x48; *p++ = 0x83; *p++ = 0xC4; *p++ = 0x08;

    if (need_map)
    {
        // Target is the per-instruction jump_wrapper buffer, whose address is
        // stable (it lives inside the persistent precomp_instr struct). Baking
        // it absolutely is safe. The wrapper does NOT touch RSP; the `add rsp,8`
        // above already restored the RSP it expects.
        uintptr_t target = (uintptr_t)(cur->reg_cache_infos.jump_wrapper);

        // mov r11, imm64(target)   (49 BB <8 bytes>)
        *p++ = 0x49; *p++ = 0xBB;
        for (int i = 0; i < 8; ++i)
            *p++ = (unsigned char)((target >> (i * 8)) & 0xFF);

        // jmp r11   (49 FF E3)
        *p++ = 0x49; *p++ = 0xFF; *p++ = 0xE3;
    }
    else
    {
        // Do NOT bake (block->code + local_addr) absolutely. The thunk executes
        // deferred (via the patched return address, after this function returns
        // and the call_reg64 chain unwinds). In that window the block can be
        // recompiled: grow_buffer/realloc_exec MOVES block->code, and local_addr
        // is reassigned. A baked absolute target then points into the old,
        // superseded (deferred-freed) buffer at a stale offset — landing in
        // zeroed/abandoned memory. Instead compute the target at RUNTIME from
        // the live struct fields, whose *addresses* are stable (both the block
        // object and the current instr persist). This mirrors build_wrapper.
        //
        // cur->local_addr is an offset into the CURRENT block's code buffer
        // (blocks[PC->addr >> 12]->code), so the two are always consistent.
        uintptr_t p_code = (uintptr_t)&block->code;
        uintptr_t p_local = (uintptr_t)&cur->local_addr;

        // mov r10, imm64(&block->code)   (49 BA <8 bytes>)
        *p++ = 0x49; *p++ = 0xBA;
        for (int i = 0; i < 8; ++i)
            *p++ = (unsigned char)((p_code >> (i * 8)) & 0xFF);

        // mov r11, [r10]        (4D 8B 1A)   r11 = block->code (live)
        *p++ = 0x4D; *p++ = 0x8B; *p++ = 0x1A;

        // mov r10, imm64(&PC->local_addr)   (49 BA <8 bytes>)
        *p++ = 0x49; *p++ = 0xBA;
        for (int i = 0; i < 8; ++i)
            *p++ = (unsigned char)((p_local >> (i * 8)) & 0xFF);

        // add r11, [r10]        (4D 03 1A)   r11 += local_addr (live)
        *p++ = 0x4D; *p++ = 0x03; *p++ = 0x1A;

        // jmp r11   (49 FF E3)
        *p++ = 0x49; *p++ = 0xFF; *p++ = 0xE3;
    }

    // Patch *return_address to jump to the thunk instead of falling through.
    *return_address = (uintptr_t)thunk_addr;
}

// dyna_stop() jumps out of recompiled code back into dyna_start(), over however many JIT
// and C frames are in between. Elsewhere that's setjmp/longjmp, which just restores the
// saved registers and abandons those frames.
//
// MSVC's x64 longjmp can't be used: it unwinds, handing the saved frame to RtlUnwindEx,
// which walks the SEH unwind tables. Recompiled code and the thunks above register no
// unwind data, so RtlVirtualUnwind faults on the first one it reaches (zeroing the
// jmp_buf's frame field doesn't avoid it). RtlRestoreContext restores registers directly,
// without unwinding, which is the semantics wanted here.
//
// Either way nothing is cleaned up, so anything the dynarec calls out to must not throw.
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

void dyna_start(void (*code)())
{
    core_executing = true;
    g_core->callbacks.core_executing_changed(core_executing);

    g_dyna_stopped = false;

    if (!dyna_save_context())
    {
        code();
    }
}

void dyna_stop()
{
    g_dyna_stopped = true;
    dyna_restore_context();
}
