/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <chrono>

#include <SDL3/SDL_audio.h>

#include "Config.hpp"

namespace SDLAudio
{

class SDLBackend
{
  public:
    SDLBackend(Config&& config);

    ~SDLBackend();

    SDLBackend(const SDLBackend&) = delete;
    SDLBackend(SDLBackend&&) = delete;

    SDLBackend& operator=(const SDLBackend&) = delete;
    SDLBackend& operator=(SDLBackend&&) = delete;

    void set_sample_rate(uint32_t sample_rate);

    void push_samples(void* src, size_t len);

    void sync_audio();

  private:
    size_t estimate_dst_frames_at_next_cb();

    void set_paused(bool paused);

    Config m_config;
    SDL_AudioDeviceID m_device_id;
    SDL_AudioSpec m_device_spec;
    int m_buffer_size;

    SDL_AudioStream* m_stream;
    SDL_AudioSpec m_input_spec;
    int m_src_target;
    bool m_paused = true;

    std::chrono::steady_clock::time_point m_last_cb_time;
};
} // namespace SDLAudio