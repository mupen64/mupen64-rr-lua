/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <atomic>
#include <chrono>

#include <SDL3/SDL_audio.h>

#include "Config.hpp"

namespace SDLAudio
{

using Timepoint = std::chrono::steady_clock::time_point;

class SDLBackend
{
  public:
    SDLBackend(Config &&config);

    ~SDLBackend();

    SDLBackend(const SDLBackend &) = delete;
    SDLBackend(SDLBackend &&) = delete;

    SDLBackend &operator=(const SDLBackend &) = delete;
    SDLBackend &operator=(SDLBackend &&) = delete;

    // Merges newly set config settings that can be set live.
    void merge_cfg_live(const Config &config2);

    // Sets the sample rate.
    void set_sample_rate(uint32_t sample_rate);

    // Tries to add new samples. Note that this may do nothing
    // if the sample buffer is already too full.
    void push_samples(void *src, size_t len);

    // Performs audio syncing/blocking.
    void sync_audio();

  private:
    // Updates any config settings that can be adjusted live.
    void update_cfg_live();

    // Note: frame = pair of L/R samples.
    // For sync/blocking, estimates how many output frames will be available
    // by the next audio callback.
    size_t estimate_dst_frames_at_next_cb();

    // Pauses/unpauses the audio stream.
    void set_paused(bool paused);

    Config m_config;
    SDL_AudioDeviceID m_device_id;
    SDL_AudioSpec m_device_spec;
    int m_buffer_size;

    SDL_AudioStream *m_stream;
    SDL_AudioSpec m_input_spec;
    int m_src_target;
    bool m_paused = true;

    std::vector<uint8_t> m_silence_buf;

    std::atomic<Timepoint> m_last_cb_time;
    std::atomic<Timepoint> m_block_until_time;
};
} // namespace SDLAudio