/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/timers.h>
#include <include/core_api.h>
#include <memory/pif.h>
#include <r4300/r4300.h>

extern int32_t m_current_vi;
extern int32_t m_current_sample;

std::chrono::duration<double, std::milli> max_vi_s_ms;

size_t frame_deltas_ptr = 0;
size_t vi_deltas_ptr = 0;

time_point last_vi_time;
time_point last_frame_time;

void core_vr_on_speed_modifier_changed()
{
    const double max_vi_s = core_vr_get_vis_per_second(ROM_HEADER.Country_code);
    max_vi_s_ms = std::chrono::duration<double, std::milli>(
    1000.0 / (max_vi_s * static_cast<double>(g_core->cfg->fps_modifier) / 100));

    last_frame_time = std::chrono::high_resolution_clock::now();
    last_vi_time = std::chrono::high_resolution_clock::now();

    for (auto& delta : g_core->g_frame_deltas)
    {
        delta = {};
    }
    for (auto& delta : g_core->g_vi_deltas)
    {
        delta = {};
    }

    frame_deltas_ptr = 0;
    vi_deltas_ptr = 0;
}

void timer_new_frame()
{
    const auto current_frame_time = std::chrono::high_resolution_clock::now();

    g_core->g_frame_deltas_mutex.lock();
    g_core->g_frame_deltas[frame_deltas_ptr] = current_frame_time - last_frame_time;
    g_core->g_frame_deltas_mutex.unlock();
    frame_deltas_ptr = (frame_deltas_ptr + 1) % core_timer_max_deltas;

    g_core->callbacks.frame();
    last_frame_time = std::chrono::high_resolution_clock::now();
}

void timer_new_vi()
{
    if (g_core->cfg->max_lag != 0 && lag_count >= g_core->cfg->max_lag)
    {
        g_core->callbacks.lag_limit_exceeded();
    }

    auto current_vi_time = std::chrono::high_resolution_clock::now();

    if (!g_vr_fast_forward && frame_advance_outstanding == 0)
    {
        static std::chrono::duration<double, std::nano> last_sleep_error;
        // if we're playing game normally with no frame advance or ff and overstepping max time between frames,
        // we need to sleep to compensate the additional time
        const auto vi_time_diff = current_vi_time - last_vi_time;
        if (!frame_advance_outstanding && vi_time_diff < max_vi_s_ms)
        {
            auto sleep_time = max_vi_s_ms - vi_time_diff;
            if (sleep_time.count() > 0 && sleep_time < std::chrono::milliseconds(700))
            {
                // we try to sleep for the overstepped time, but must account for sleeping inaccuracies
                const auto goal_sleep = max_vi_s_ms - vi_time_diff -
                last_sleep_error;
                const auto start_sleep =
                std::chrono::high_resolution_clock::now();
                std::this_thread::sleep_for(goal_sleep);
                const auto end_sleep =
                std::chrono::high_resolution_clock::now();

                // sleeping inaccuracy is difference between actual time spent sleeping and the goal sleep
                // this value isnt usually too large
                last_sleep_error = end_sleep - start_sleep - goal_sleep;

                // This value is used later to calculate the deltas so we need to reassign it here to cut out the sleep time from the current delta
                current_vi_time = std::chrono::high_resolution_clock::now();
            }
            else
            {
                // sleep time is unreasonable, log it and reset related state
                const auto casted = std::chrono::duration_cast<
                                    std::chrono::milliseconds>(sleep_time)
                                    .count();
                g_core->log_info(std::format(L"Invalid timer: %lld ms", casted));
                sleep_time = sleep_time.zero();
            }
        }
    }

    g_core->g_vi_deltas_mutex.lock();
    g_core->g_vi_deltas[vi_deltas_ptr] = current_vi_time - last_vi_time;
    g_core->g_vi_deltas_mutex.unlock();
    vi_deltas_ptr = (vi_deltas_ptr + 1) % core_timer_max_deltas;

    last_vi_time = std::chrono::high_resolution_clock::now();
}
