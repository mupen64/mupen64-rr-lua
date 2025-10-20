/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <Core.h>
#include <r4300/timers.h>
#include <include/core_api.h>
#include <memory/pif.h>
#include <r4300/r4300.h>

struct timer_state
{
    std::chrono::duration<double, std::milli> max_vi_s_ms;

    size_t frame_deltas_ptr{};
    size_t vi_deltas_ptr{};

    time_point last_vi_time{};
    time_point last_frame_time{};

    core_timer_delta frame_deltas[core_timer_max_deltas]{};
    std::mutex frame_deltas_mtx{};

    core_timer_delta vi_deltas[core_timer_max_deltas]{};
    std::mutex vi_deltas_mtx{};
};

static timer_state timer{};

/**
 * \brief Computes the average rate of entries in the time queue per second (e.g.: FPS from frame deltas)
 * \param times A circular buffer of deltas
 * \return The average rate per second from the delta in the queue
 */
static float get_rate_per_second_from_deltas(const std::span<core_timer_delta> &times)
{
    size_t count = 0;
    float sum = 0.0f;
    for (const auto &time : times)
    {
        if (time.count() > 0)
        {
            sum += (float)time.count() / 1000000.0f;
            count++;
        }
    }

    if (count <= 0)
    {
        return 0.0f;
    }

    return 1000.0f / (sum / (float)count);
}

void timer_on_speed_modifier_changed()
{
    const double max_vi_s = g_ctx.vr_get_vis_per_second(ROM_HEADER.Country_code);
    timer.max_vi_s_ms = std::chrono::duration<double, std::milli>(
        1000.0 / (max_vi_s * static_cast<double>(g_core->cfg->fps_modifier) / 100));

    timer.last_frame_time = std::chrono::high_resolution_clock::now();
    timer.last_vi_time = std::chrono::high_resolution_clock::now();

    for (auto &delta : timer.frame_deltas)
    {
        delta = {};
    }
    for (auto &delta : timer.vi_deltas)
    {
        delta = {};
    }

    timer.frame_deltas_ptr = 0;
    timer.vi_deltas_ptr = 0;
}

void timer_new_frame()
{
    const auto current_frame_time = std::chrono::high_resolution_clock::now();

    timer.frame_deltas_mtx.lock();
    timer.frame_deltas[timer.frame_deltas_ptr] = current_frame_time - timer.last_frame_time;
    timer.frame_deltas_mtx.unlock();
    timer.frame_deltas_ptr = (timer.frame_deltas_ptr + 1) % core_timer_max_deltas;

    g_core->callbacks.frame();
    timer.last_frame_time = std::chrono::high_resolution_clock::now();
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
        const auto vi_time_diff = current_vi_time - timer.last_vi_time;
        if (!frame_advance_outstanding && vi_time_diff < timer.max_vi_s_ms)
        {
            auto sleep_time = timer.max_vi_s_ms - vi_time_diff;
            if (sleep_time.count() > 0 && sleep_time < std::chrono::milliseconds(700))
            {
                // we try to sleep for the overstepped time, but must account for sleeping inaccuracies
                const auto goal_sleep = timer.max_vi_s_ms - vi_time_diff - last_sleep_error;
                const auto start_sleep = std::chrono::high_resolution_clock::now();
                std::this_thread::sleep_for(goal_sleep);
                const auto end_sleep = std::chrono::high_resolution_clock::now();

                // sleeping inaccuracy is difference between actual time spent sleeping and the goal sleep
                // this value isnt usually too large
                last_sleep_error = end_sleep - start_sleep - goal_sleep;

                // This value is used later to calculate the deltas so we need to reassign it here to cut out the sleep
                // time from the current delta
                current_vi_time = std::chrono::high_resolution_clock::now();
            }
            else
            {
                // sleep time is unreasonable, log it and reset related state
                const auto casted = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time).count();
                g_core->log_info(std::format("Invalid timer: %lld ms", casted));
                sleep_time = sleep_time.zero();
            }
        }
    }

    timer.vi_deltas_mtx.lock();
    timer.vi_deltas[timer.vi_deltas_ptr] = current_vi_time - timer.last_vi_time;
    timer.vi_deltas_mtx.unlock();
    timer.vi_deltas_ptr = (timer.vi_deltas_ptr + 1) % core_timer_max_deltas;

    timer.last_vi_time = std::chrono::high_resolution_clock::now();
}

void timer_get_timings(float &fps, float &vis)
{
    timer.frame_deltas_mtx.lock();
    fps = get_rate_per_second_from_deltas(timer.frame_deltas);
    timer.frame_deltas_mtx.unlock();

    timer.vi_deltas_mtx.lock();
    vis = get_rate_per_second_from_deltas(timer.vi_deltas);
    timer.vi_deltas_mtx.unlock();
}
