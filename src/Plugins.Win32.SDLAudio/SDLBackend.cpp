/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SDLBackend.hpp"
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace SDLAudio
{
SDLBackend::SDLBackend(Config &&config) : m_config(config)
{

    // request default audio settings

    m_device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_device_id) throw std::runtime_error(SDL_GetError());

    if (!SDL_GetAudioDeviceFormat(m_device_id, &m_device_spec, &m_buffer_size))
        throw std::runtime_error(SDL_GetError());

    m_input_spec = SDL_AudioSpec{
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = (int)m_config.default_sample_rate,
    };

    // Create and bind an audio stream
    m_stream = SDL_CreateAudioStream(&m_input_spec, nullptr);
    if (!m_stream) throw std::runtime_error(SDL_GetError());
    if (!SDL_BindAudioStream(m_device_id, m_stream)) throw std::runtime_error(SDL_GetError());

    // SDL3 starts audio devices paused.
    m_paused = true;

    // set the target buffer size for audio synchronization
    m_src_target = std::max((int)config.src_buffer_target, m_buffer_size);
    // setup a callback to track when HW requests samples from us
    if (!SDL_SetAudioStreamGetCallback(
            m_stream,
            [](void *userdata, SDL_AudioStream * /*stream*/, int /*additional_amount*/, int /*total_amount*/) {
                auto *self = (SDLBackend *)userdata;

                self->m_last_cb_time = std::chrono::steady_clock::now();
            },
            this))
    {
        throw std::runtime_error(SDL_GetError());
    }
}

SDLBackend::~SDLBackend()
{
}

void SDLBackend::set_sample_rate(uint32_t sample_rate)
{
    m_input_spec.freq = (int)sample_rate;
    SDL_SetAudioStreamFormat(m_stream, &m_input_spec, nullptr);
}

void SDLBackend::push_samples(void *src, size_t len)
{
    SDL_PutAudioStreamData(m_stream, src, (int)len);
}

void SDLBackend::sync_audio()
{
    constexpr size_t TIME_TOLERANCE_MS = 10;
    namespace chr = std::chrono;
    using clock_frac = std::chrono::steady_clock::period;

    size_t expected_frames = estimate_dst_frames_at_next_cb();
    size_t max_target_frames = m_src_target + ((size_t)m_device_spec.freq * TIME_TOLERANCE_MS / 1000);

    if (m_config.audio_sync && (expected_frames >= max_target_frames))
    {
        // figure out how long we need to delay the core.
        intmax_t wait_clock_period =
            ((expected_frames - m_src_target) * clock_frac::den) / ((intmax_t)m_device_spec.freq * clock_frac::num);
        auto wait_duration = chr::steady_clock::duration(wait_clock_period);

        // If the core is ahead, have it wait here to sync up with the audio.
        set_paused(false);
        std::this_thread::sleep_for(wait_duration);
    }
    else
    {
        // pause if we don't have enough audio.
        set_paused(expected_frames < m_buffer_size);
    }
}

size_t SDLBackend::estimate_dst_frames_at_next_cb()
{
    namespace chr = std::chrono;
    using clock_frac = std::chrono::steady_clock::period;

    auto now = chr::steady_clock::now();

    // find the current number of available output frames
    uint32_t bytes_per_frame = (((uint32_t)m_device_spec.format) & 0xFF) / 8;
    int dst_bytes = SDL_GetAudioStreamAvailable(m_stream);
    if (dst_bytes < 0)
        throw std::runtime_error(SDL_GetError());
    uint32_t dst_frames = dst_bytes / bytes_per_frame;

    // assume that our audio buffer is filled fast enough to have a full buffer by the next call.
    // we can use this to estimate when the next call should be.
    intmax_t time_to_next_call =
        ((intmax_t)m_buffer_size * clock_frac::den) / ((intmax_t)m_device_spec.freq * clock_frac::num);
    auto predicted_next_cb_time = m_last_cb_time + chr::steady_clock::duration(time_to_next_call);

    // if there's still time to go, count in however many samples should be added between now and callback time
    if (now < predicted_next_cb_time)
    {
        dst_frames += ((predicted_next_cb_time - now).count() * m_device_spec.freq * clock_frac::num) / clock_frac::den;
    }
    return dst_frames;
}

void SDLBackend::set_paused(bool paused)
{
    if (paused)
        SDL_PauseAudioDevice(m_device_id);
    else
        SDL_ResumeAudioDevice(m_device_id);
}
} // namespace SDLAudio