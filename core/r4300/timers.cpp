/***************************************************************************
                                                  timers.c  -  description
                                                         -------------------
        copyright            : (C) 2003 by ShadowPrince
        email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "timers.h"
#include <core/memory/pif.h>
#include <core/r4300/r4300.h>
#include <core/r4300/vcr.h>
#include <shared/Config.hpp>
#include <shared/Messenger.h>
#include <shared/services/LoggingService.h>

bool frame_changed = true;
extern int32_t m_current_vi;
extern int32_t m_current_sample;

std::chrono::duration<double, std::milli> max_vi_s_ms;

timer_delta g_frame_deltas[max_deltas]{};
std::mutex g_frame_deltas_mutex;

timer_delta g_vi_deltas[max_deltas]{};
std::mutex g_vi_deltas_mutex;

size_t frame_deltas_ptr = 0;
size_t vi_deltas_ptr = 0;

time_point last_vi_time;
time_point last_frame_time;

void timer_init(int32_t speed_modifier, t_rom_header *rom_header) {
  const double max_vi_s = get_vis_per_second(rom_header->Country_code);
  max_vi_s_ms = std::chrono::duration<double, std::milli>(
      1000.0 / (max_vi_s * static_cast<double>(speed_modifier) / 100));

  last_frame_time = std::chrono::high_resolution_clock::now();
  last_vi_time = std::chrono::high_resolution_clock::now();

  for (auto &delta : g_frame_deltas) {
    delta = {};
  }
  for (auto &delta : g_vi_deltas) {
    delta = {};
  }

  frame_deltas_ptr = 0;
  vi_deltas_ptr = 0;

  Messenger::broadcast(Messenger::Message::SpeedModifierChanged,
                       speed_modifier);
}

void timer_new_frame() {
  const auto current_frame_time = std::chrono::high_resolution_clock::now();

  g_frame_deltas_mutex.lock();
  g_frame_deltas[frame_deltas_ptr] = current_frame_time - last_frame_time;
  g_frame_deltas_mutex.unlock();
  frame_deltas_ptr = (frame_deltas_ptr + 1) % max_deltas;

  frame_changed = true;
  last_frame_time = std::chrono::high_resolution_clock::now();
}

void timer_new_vi() {
  if (g_config.max_lag != 0 && lag_count >= g_config.max_lag) {
    Messenger::broadcast(Messenger::Message::LagLimitExceeded, nullptr);
  }

  auto current_vi_time = std::chrono::high_resolution_clock::now();

  if (!g_vr_fast_forward) {
    static std::chrono::duration<double, std::nano> last_sleep_error;
    // if we're playing game normally with no frame advance or ff and
    // overstepping max time between frames, we need to sleep to compensate the
    // additional time
    const auto vi_time_diff = current_vi_time - last_vi_time;
    if (!frame_advancing && vi_time_diff < max_vi_s_ms) {
      auto sleep_time = max_vi_s_ms - vi_time_diff;
      if (sleep_time.count() > 0 &&
          sleep_time < std::chrono::milliseconds(700)) {
        // we try to sleep for the overstepped time, but must account for
        // sleeping inaccuracies
        const auto goal_sleep = max_vi_s_ms - vi_time_diff - last_sleep_error;
        const auto start_sleep = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(goal_sleep);
        const auto end_sleep = std::chrono::high_resolution_clock::now();

        // sleeping inaccuracy is difference between actual time spent sleeping
        // and the goal sleep this value isnt usually too large
        last_sleep_error = end_sleep - start_sleep - goal_sleep;

        // This value is used later to calculate the deltas so we need to
        // reassign it here to cut out the sleep time from the current delta
        current_vi_time = std::chrono::high_resolution_clock::now();
      } else {
        // sleep time is unreasonable, log it and reset related state
        const auto casted =
            std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time)
                .count();
        g_core_logger->info("Invalid timer: %lld ms", casted);
        sleep_time = sleep_time.zero();
      }
    }
  }

  g_vi_deltas_mutex.lock();
  g_vi_deltas[vi_deltas_ptr] = current_vi_time - last_vi_time;
  g_vi_deltas_mutex.unlock();
  vi_deltas_ptr = (vi_deltas_ptr + 1) % max_deltas;

  last_vi_time = std::chrono::high_resolution_clock::now();
}
