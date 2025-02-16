/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// #include "../config.h"

#include "stdafx.h"
#include <Core.h>
#include <r4300/interrupt.h>
#include <memory/memory.h>
#include <r4300/r4300.h>
#include <r4300/macros.h>
#include <r4300/exception.h>
#include <r4300/vcr.h>
#include <r4300/timers.h>
#include <memory/pif.h>

typedef struct _interrupt_queue {
    int32_t type;
    uint32_t count;
    struct _interrupt_queue* next;
} interrupt_queue;

static interrupt_queue* q = NULL;

interrupt_queue g_pool[128]{};
uint8_t g_pool_used[sizeof(g_pool)]{};
size_t g_known_unused_index = SIZE_MAX;

int interupt_unsafe_state = 0;

/**
 * Allocates an item in the interrupt pool.
 */
interrupt_queue* pool_alloc()
{
    size_t unused_index = SIZE_MAX;

    // OPTIMIZATION: If we know that there is an unused index, use it
    if (g_known_unused_index != SIZE_MAX)
    {
        unused_index = g_known_unused_index;
    }
    else
    {
        for (int32_t i = 0; i < sizeof(g_pool); ++i)
        {
            if (g_pool_used[i] == false)
            {
                unused_index = i;
                break;
            }
        }
        assert(unused_index != SIZE_MAX);
    }

    g_pool_used[unused_index] = true;
    g_known_unused_index = SIZE_MAX;

    return &g_pool[unused_index];
}

/**
 * Frees an interrupt from the pool, allowing it to be reused.
 */
void pool_free(const interrupt_queue* ptr)
{
    const auto index_in_pool = ptr - (interrupt_queue*)&g_pool;

#ifdef _DEBUG
    size_t index = SIZE_MAX;
    for (int32_t i = 0; i < sizeof(g_pool); ++i)
    {
        if (&g_pool[i] == ptr)
        {
            index = i;
            break;
        }
    }

    assert(index != SIZE_MAX);
    assert(index == index_in_pool);
#endif

    g_pool_used[index_in_pool] = false;
    g_known_unused_index = index_in_pool;
}

/**
 * Clears the pool.
 */
void pool_clear()
{
    memset(g_pool_used, 0, std::size(g_pool_used));
    g_known_unused_index = SIZE_MAX;
}

void clear_queue()
{
    while (q != NULL)
    {
        interrupt_queue* aux = q->next;
        q = aux;
    }
    pool_clear();
}

void print_queue()
{
    interrupt_queue* aux;
    // if (Count < 0x7000000) return;
    g_core->logger->info("------------------ {:#06x}", (uint32_t)core_Count);
    aux = q;
    while (aux != NULL)
    {
        g_core->logger->info("Count:{:#06x}, {:#06x}", (uint32_t)aux->count, aux->type);
        aux = aux->next;
    }
    g_core->logger->info("------------------");
    // getchar();
}

static int32_t SPECIAL_done = 0;

/// <summary>
/// Checks if evt1 will happen before evt2
/// </summary>
/// <param name="evt1"></param>
/// <param name="evt2"></param>
/// <param name="type2"></param>
/// <returns></returns>
int32_t before_event(uint32_t evt1, uint32_t evt2, int32_t type2)
{
    if (evt1 - core_Count < 0x80000000)
    {
        if (evt2 - core_Count < 0x80000000)
        {
            if ((evt1 - core_Count) < (evt2 - core_Count))
                return 1;
            else
                return 0;
        }
        else
        {
            if ((core_Count - evt2) < 0x10000000)
            {
                switch (type2)
                {
                case SPECIAL_INT:
                    if (SPECIAL_done)
                        return 1;
                    else
                        return 0;
                    break;
                default:
                    return 0;
                }
            }
            else
                return 1;
        }
    }
    else
        return 0;
}

/// <summary>
/// Adds interrupt to queue that will fire after certain amount of time (Count register cycles)
/// </summary>
/// <param name="type">type of interrupt</param>
/// <param name="delay">how much to wait</param>
void add_interrupt_event(int32_t type, uint32_t delay)
{
    uint32_t count = core_Count + delay /**2*/;
    int32_t special = 0;

    if (type == SPECIAL_INT /*|| type == COMPARE_INT*/)
        special = 1;
    if (core_Count > 0x80000000)
        SPECIAL_done = 0;

    if (get_event(type))
    {
        g_core->logger->info("two events of type {:#06x} in queue", type);
        print_queue();
    }
    interrupt_queue* aux = q;

    // if (type == PI_INT)
    //{
    // delay = 0;
    // count = Count + delay/**2*/;
    // }

    if (q == NULL)
    {
        q = pool_alloc();
        q->next = NULL;
        q->count = count;
        q->type = type;
        next_interrupt = q->count;
        // print_queue();
        return;
    }

    // finds place in queue to insert the interrupt ( its sorted )
    if (before_event(count, q->count, q->type) && !special)
    {
        q = pool_alloc();
        q->next = aux;
        q->count = count;
        q->type = type;
        next_interrupt = q->count;
        // print_queue();
        return;
    }

    while (aux->next != NULL && (!before_event(count, aux->next->count, aux->next->type) || special))
        aux = aux->next;

    if (aux->next == NULL)
    {
        aux->next = pool_alloc();
        aux = aux->next;
        aux->next = NULL;
        aux->count = count;
        aux->type = type;
    }
    else
    {
        interrupt_queue* aux2;
        if (type != SPECIAL_INT)
            while (aux->next != NULL && aux->next->count == count)
                aux = aux->next;
        aux2 = aux->next;
        aux->next = pool_alloc();
        aux = aux->next;
        aux->next = aux2;
        aux->count = count;
        aux->type = type;
    }
    /*if (q->count > Count || (Count - q->count) < 0x80000000)
      next_interrupt = q->count;
    else
      next_interrupt = 0;*/
    // print_queue();
}

/// <summary>
/// Add new interrupt that will fire when Count register reaches given value
/// See add_interrupt_event for simmilar function
/// </summary>
/// <param name="type">type of interrupt</param>
/// <param name="count">when to make this interrupt happen</param>
void add_interrupt_event_count(int32_t type, uint32_t count)
{
    add_interrupt_event(type, (count - core_Count) /*/2*/);
}

void remove_interrupt_event()
{
    interrupt_queue* aux = q->next;
    if (q->type == SPECIAL_INT)
        SPECIAL_done = 1;
    pool_free(q);
    q = aux;
    if (q != NULL && (q->count > core_Count || (core_Count - q->count) < 0x80000000))
        next_interrupt = q->count;
    else
        next_interrupt = 0;
}

/// <summary>
/// Check if event of this type already exists in queue (for some reason this is forbidden)
/// </summary>
/// <param name="type">type of interrupt, see interrupt.h</param>
/// <returns></returns>
uint32_t get_event(int32_t type)
{
    interrupt_queue* aux = q;
    if (q == NULL)
        return 0;
    if (q->type == type)
        return q->count;
    while (aux->next != NULL && aux->next->type != type)
        aux = aux->next;
    if (aux->next != NULL)
        return aux->next->count;
    return 0;
}

/// <summary>
/// finds and removes this type of event from queue
/// </summary>
/// <param name="type">interrupt type to find</param>
void remove_event(int32_t type)
{
    interrupt_queue* aux = q;
    if (q == NULL)
        return;
    if (q->type == type)
    {
        aux = aux->next;
        pool_free(q);
        q = aux;
        return;
    }
    while (aux->next != NULL && aux->next->type != type)
        aux = aux->next;
    if (aux->next != NULL) // it's a type int32_t
    {
        interrupt_queue* aux2 = aux->next->next;
        pool_free(aux->next);
        aux->next = aux2;
    }
}

void translate_event_queue(uint32_t base)
{
    interrupt_queue* aux;
    remove_event(COMPARE_INT);
    remove_event(SPECIAL_INT);
    aux = q;
    while (aux != NULL)
    {
        aux->count = (aux->count - core_Count) + base;
        aux = aux->next;
    }
    add_interrupt_event_count(COMPARE_INT, core_Compare);
    add_interrupt_event_count(SPECIAL_INT, 0);
}

int32_t save_eventqueue_infos(char* buf)
{
#ifdef _DEBUG
    if (get_event(SI_INT))
        g_core->logger->info("SI_INT in queue, good");
    else
        g_core->logger->info("SI_INT not found");
#endif
    int32_t len = 0;
    interrupt_queue* aux = q;
    if (q == NULL)
    {
        *((uint32_t*)&buf[0]) = 0xFFFFFFFF;
        return 4;
    }
    while (aux != NULL)
    {
        memcpy(buf + len, &aux->type, 4);
        memcpy(buf + len + 4, &aux->count, 4);
        len += 8;
        aux = aux->next;
    }
    *((uint32_t*)&buf[len]) = 0xFFFFFFFF;
    return len + 4;
}

void load_eventqueue_infos(char* buf)
{
    int32_t len = 0;
    clear_queue();
    while (*((uint32_t*)&buf[len]) != 0xFFFFFFFF)
    {
        int32_t type = *((uint32_t*)&buf[len]);
        uint32_t count = *((uint32_t*)&buf[len + 4]);
        add_interrupt_event_count(type, count);
        len += 8;
    }
}

void init_interrupt()
{
    SPECIAL_done = 1;
    next_vi = next_interrupt = 5000;
    vi_register.vi_delay = next_vi;
    vi_field = 0;
    clear_queue();
    add_interrupt_event_count(VI_INT, next_vi);
    add_interrupt_event_count(SPECIAL_INT, 0);
}

void check_interrupt()
{
    // checks if MI register has any bit set (after masking)
    // if yes, sets the pending RCP bit

    // same thing is done in gen_interrupt, but this is needed so that the bit is cleared properly
    if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
        core_Cause = (core_Cause | 0x400) & 0xFFFFFF83; // 0x400 is CAUSE_IP3 (rcp interrupt pending)
    else
        core_Cause &= ~0x400;
    if ((core_Status & 7) != 1)
        return;
    // if any of the interrupts is pending, add a check interrupt
    // (which does nothing itself but makes cpu jump to general exception vector)
    if (core_Status & core_Cause & 0xFF00)
    {
        if (q == NULL)
        {
            q = pool_alloc();
            q->next = NULL;
            q->count = core_Count;
            q->type = CHECK_INT;
        }
        else
        {
            interrupt_queue* aux = pool_alloc();
            aux->next = q;
            aux->count = core_Count;
            aux->type = CHECK_INT;
            q = aux;
        }
        next_interrupt = core_Count;
    }
}


void gen_interrupt()
{
    // auto starttime = std::chrono::high_resolution_clock::now();
    // if (!skip_jump)
    // g_core->logger->info("interrupt:{:#06x} ({:#06x})", q->type, Count);

    // dyna_stop()��longjmp()���邽�߁A�������O�ŃR���X�g���N�g����ƃf�X�g���N�^���Ă΂�Ȃ�
    if (stop)
    {
        dyna_stop();
    }

    if (skip_jump /*&& !dynacore*/)
    {
        if (q->count > core_Count || (core_Count - q->count) < 0x80000000)
            next_interrupt = q->count;
        else
            next_interrupt = 0;
        if (interpcore)
        {
            /*if ((Cause & (2 << 2)) && (Cause & 0x80000000))
              interp_addr = skip_jump+4;
            else*/
            interp_addr = skip_jump;
            last_addr = interp_addr;
        }
        else
        {
            /*if ((Cause & (2 << 2)) && (Cause & 0x80000000))
              jump_to(skip_jump+4);
            else*/
            uint32_t dest = skip_jump;
            skip_jump = 0;
            jump_to(dest);
            last_addr = PC->addr;
        }
        skip_jump = 0;
        return;
    }
    auto type = q->type;
    switch (q->type)
    {
    case SPECIAL_INT: // does nothing, spammed when Count is close to rolling over
        // g_core->logger->info("SPECIAL, count: {:#06x}", q->count);
        if (core_Count > 0x10000000)
            return;
        remove_interrupt_event();
        add_interrupt_event_count(SPECIAL_INT, 0);
        return;
        break;

    case VI_INT:
        {
            lag_count++;

            // NOTE: It's ok to not update screen when lagging, doesn't cause any obvious issues
            auto skip = (g_core->cfg->skip_rendering_lag && lag_count > 1) || g_vr_frame_skipped;
            auto update = g_core->cfg->render_throttling ? (screen_invalidated ? !skip : false) : true;

            // NOTE: When frame advancing, screen_invalidated has a higher change of being false despite the fact it should be true
            // The update-limiting logic doesn't apply in frameadvance because there are no high-frequency updates
            if (update || frame_advancing)
            {
                g_core->update_screen();
                screen_invalidated = false;
            }

            g_core->callbacks.vi();

            vcr_on_vi();

            timer_new_vi();

            if (vi_register.vi_v_sync == 0)
                vi_register.vi_delay = 500000;
            else
                vi_register.vi_delay = ((vi_register.vi_v_sync + 1) * (1500 * g_core->cfg->counter_factor));
            // this is the place
            next_vi += vi_register.vi_delay;
            if (vi_register.vi_status & 0x40)
                vi_field = 1 - vi_field;
            else
                vi_field = 0;

            remove_interrupt_event();
            add_interrupt_event_count(VI_INT, next_vi);
            //++frame_count;
            // total_vi += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - starttime).count();
            MI_register.mi_intr_reg |= 0x08;
            break;
        }
    case COMPARE_INT: // game can set Compare register to some value, and make a timer like that
        // g_core->logger->info("COMPARE, count: {:#06x}", q->count);
        remove_interrupt_event();
        core_Count += 2;
        add_interrupt_event_count(COMPARE_INT, core_Compare);
        core_Count -= 2;
        break;

    case CHECK_INT: // fake interrupt used to trigger exception handler (when interrupt is pending)
        // g_core->logger->info("CHECK, count: {:#06x}", q->count);
        remove_interrupt_event();
        break;

    // serial interface, means that PIF copy/write happened (controllers)
    // notice this is spammed a lot during loading
    case SI_INT:
        // g_core->logger->info("SI, count: {:#06x}", q->count);
        PIF_RAMb[0x3F] = 0x0;
        remove_interrupt_event();
        MI_register.mi_intr_reg |= 0x02;
        si_register.si_status |= 0x1000;
        break;

    // peripherial interface, dma between cartridge and rdram finished
    case PI_INT:
        // g_core->logger->info("PI, count: {:#06x}", q->count);
        remove_interrupt_event();
        MI_register.mi_intr_reg |= 0x10;
        pi_register.read_pi_status_reg &= ~3; // PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY clear
        break;

    case AI_INT:
        // g_core->logger->info("AI, count: {:#06x}", q->count);
        if (ai_register.ai_status & 0x80000000) // full
        {
            uint32_t ai_event = get_event(AI_INT);
            remove_interrupt_event();
            ai_register.ai_status &= ~0x80000000;
            ai_register.current_delay = ai_register.next_delay;
            ai_register.current_len = ai_register.next_len;
            // add_interrupt_event(AI_INT, ai_register.next_delay/**2*/);
            add_interrupt_event_count(AI_INT, ai_event + ai_register.next_delay);

            MI_register.mi_intr_reg |= 0x04;
            // apparently this should be moved to when AIlen is changed (when sound start splaying)
        }
        else
        {
            remove_interrupt_event();
            ai_register.ai_status &= ~0x40000000;

            //-------
            MI_register.mi_intr_reg |= 0x04; // this too
            // return;
        }
        break;

    case SP_INT: // related to rsp
        // g_core->logger->info("SP, count: {:#06x}", q->count);
        remove_interrupt_event();
        sp_register.sp_status_reg |= 0x303;
        // sp_register.signal1 = 1;
        sp_register.signal2 = 1;
        sp_register.broke = 1;
        sp_register.halt = 1;

        if (!sp_register.intr_break)
            return;
        MI_register.mi_intr_reg |= 0x01;
        break;

    case DP_INT:
        // g_core->logger->info("DP, count: {:#06x}", q->count);
        remove_interrupt_event();
        dpc_register.dpc_status &= ~2;
        dpc_register.dpc_status |= 0x81;
        MI_register.mi_intr_reg |= 0x20;
        break;

    case HW2_INT:
        // Hardware Interrupt 2 -- remove interrupt event from queue
        remove_interrupt_event();
        // setup r4300 Status flags: reset TS, and SR, set IM2
        core_Status = (core_Status & ~0x00380000) | 0x1000;
        core_Cause = (core_Cause | 0x1000) & 0xFFFFFF83;
        /* the exception_general() call below will jump to the interrupt vector (0x80000180) and setup the
         * interpreter or dynarec
         */
        break;

    case NMI_INT:
        // Non Maskable Interrupt -- remove interrupt event from queue
        remove_interrupt_event();
        // setup r4300 Status flags: reset TS and SR, set BEV, ERL, and SR
        core_Status = (core_Status & ~0x00380000) | 0x00500004;
        core_Cause = 0x00000000;
        // simulate the soft reset code which would run from the PIF ROM
        r4300_reset_soft();
        // clear all interrupts, reset interrupt counters back to 0
        core_Count = 0;
        init_interrupt();
        // clear the audio status register so that subsequent write_ai() calls will work properly
        ai_register.ai_status = 0;
        // reset the r4300 internal state
        if (interpcore) /* pure interpreter only */
        {
            // set ErrorEPC with last instruction address and set next instruction address to reset vector
            core_ErrorEPC = interp_addr;
            interp_addr = 0xa4000040;
            last_addr = interp_addr;
        }
        else /* decode-cached interpreter or dynamic recompiler */
        {
            int i;
            // clear all the compiled instruction blocks
            for (i = 0; i < 0x100000; i++)
            {
                if (blocks[i])
                {
                    free_block(blocks[i]);
                    free(blocks[i]);
                    blocks[i] = NULL;
                }
            }
            // clear all the compiled instruction blocks and re-initialize
            free_blocks();
            init_blocks();
            // jump to the start
            core_ErrorEPC = PC->addr;
            jump_to(0xa4000040);
            last_addr = PC->addr;
        }
        // adjust ErrorEPC if we were in a delay slot, and clear the delay_slot and dyna_interp flags
        if (delay_slot == 1 || delay_slot == 3)
        {
            core_ErrorEPC -= 4;
        }
        delay_slot = 0;
        dyna_interp = 0;
        return;

    default:
        // g_core->logger->info("!!!!!!!!!!DEFAULT");
        remove_interrupt_event();
        break;
    }

    if (type == COMPARE_INT)
        core_Cause = (core_Cause | 0x8000) & 0xFFFFFF83; // COMPARE interrupt to be pending
    if (type != CHECK_INT)
    {
        if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
            core_Cause = (core_Cause | 0x400) & 0xFFFFFF83; // RCP interrupt is pending
        else
            return;
        if ((core_Status & 7) != 1)
            return; // if interrupts shouldn't be handled, return
        if (!(core_Status & core_Cause & 0xFF00))
            return; // check if there is any pending interrupt that isn't masked away
    }


    exception_general();
}
