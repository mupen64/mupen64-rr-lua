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

// NOTE: dynarec isn't compatible with the game debugger

// The dyna_jump thunk is a small piece of generated code that repoints the
// call's return address to the dynarec target AND restores RSP.
//
// Situation before dyna_jump:
//   - Recompiled code called some C function via call_reg64
//   - call_reg64's prologue: push r11 (RSP now 0 mod 16)
//   - call_reg64's call into the C function: RSP now 8 mod 16 (ABI-correct for callee)
//   - dyna_jump is called from within that C call chain, again via call_reg64
//
// Stack inside dyna_jump on entry:
//   RSP = Y, Y%16==8 (callee view)
//   [Y] = return address (into call_reg64's caller, i.e. the C function that called dyna_jump)
//   [Y+8] = saved r11 (from that call_reg64's push r11) — note: my fixed call_reg64 does push r11 ONLY (no sub rsp,8)
//   [Y+16] = stuff from the C function's frame
//
// After dyna_jump's C `ret`:
//   Returns to the C caller. If we ALSO patched *return_address (from the OUTER call_reg64),
//   the outer call's retaddr now points to the thunk.
//
// Thunk's job:
//   add rsp, 8     — undo the push r11 from the outer call_reg64 (RSP is now caller's entry RSP)
//   mov r11, target
//   jmp r11        — enter target with restored RSP
//
// For need_map=true: target is a jump_wrapper that begins with `sub rsp, 8`.
//   That sub is correct if we enter the wrapper with RSP=original_call_reg64_entry_RSP.
//   The wrapper itself handles the rest and jumps to the real instruction code.
//
// Thunk operation:
//   Entered via CPU `ret` from C function. Wait, but this is generated dyna_jump, not a thunk!
//   dyna_jump writes the thunk into jump_thunk_buf, patches *return_address = thunk_buf_addr,
//   then does C ret. The C ret goes to the outer C caller (NOT the thunk).
//   The OUTER call_reg64's ret eventually pops the patched retaddr = thunk_buf_addr.
//   Control reaches the add_rsp_8 + jmp_target thunk.

// Thunk slots are handed out from a single pre-allocated executable pool that
// rotates. The previous implementation called malloc_exec(64) on EVERY dyna_jump,
// which on Windows maps to VirtualAlloc(MEM_RESERVE|MEM_COMMIT): each call leaked
// a 64KB address-space reservation + a committed page and was never freed (except
// at shutdown). dyna_jump runs for every register-cache-mapped jump — millions of
// times — so the process eventually exhausts commit/address space, VirtualAlloc
// returns NULL, and the very first byte-store in dyna_jump faults on a null pointer
// (write to address 0).
//
// A thunk is consumed almost immediately: dyna_jump writes it, patches the outer
// call_reg64's return-address slot, and returns; the thunk executes as soon as that
// call_reg64's `ret` pops the patched slot while the C call chain unwinds. No other
// dyna_jump can run in that window (we are returning up the stack, not entering new
// recompiled jumps), so slots can be safely reused. The rotating pool keeps a large
// margin (NUM_SLOTS) purely as defense-in-depth against any unexpected reentrancy.
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
    bool need_map = PC->reg_cache_infos.need_map;

    unsigned char* p = jump_thunk_buf();
    unsigned char* thunk_addr = p;  // Save start address for patching return_address

    // add rsp, 8   (48 83 C4 08)  — undo the push r11 from call_reg64
    *p++ = 0x48; *p++ = 0x83; *p++ = 0xC4; *p++ = 0x08;

    if (need_map)
    {
        // Target is the per-instruction jump_wrapper buffer, whose address is
        // stable (it lives inside the persistent precomp_instr struct). Baking
        // it absolutely is safe. The wrapper's own prologue does sub rsp,8, so
        // we enter it with the original RSP, which is what it expects.
        uintptr_t target = (uintptr_t)(PC->reg_cache_infos.jump_wrapper);

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
        // object and PC persist). This mirrors build_wrapper.
        //
        // PC->local_addr is an offset into the buffer of the block that CONTAINS
        // PC — blocks[PC->addr >> 12], not the global `actual` (which lags after
        // direct patched cross-block jumps that bypass jump_to_func).
        precomp_block* block = blocks[PC->addr >> 12];
        uintptr_t p_code = (uintptr_t)&block->code;
        uintptr_t p_local = (uintptr_t)&PC->local_addr;

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

jmp_buf g_jmp_state;

void dyna_start(void (*code)())
{
    core_executing = true;
    g_core->callbacks.core_executing_changed(core_executing);
    g_core->log_info(std::format("core_executing: {}", (bool)core_executing));
    if (setjmp(g_jmp_state) == 0)
    {
        code();
    }
}

void dyna_stop()
{
    longjmp(g_jmp_state, 1);
}
