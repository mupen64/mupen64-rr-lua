/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SDLBackend.hpp"
#include "Main.hpp"
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <format>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

static void swap_channels(void *data, size_t len)
{
    // This should always be true
    assert(len % 4 == 0);

    auto *end = (uint16_t *)((uint8_t *)data + len);
    for (auto *ptr = (uint16_t *)data; ptr != end; ptr += 2)
    {
        std::swap(ptr[0], ptr[1]);
    }
}

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

    g_ef->log_info(std::format(L"Opened default audio device, buffer size = {}, target = {}", m_buffer_size,
                               config.src_buffer_target)
                       .c_str());

    // Pause the stream initially.
    set_paused(true);

    // set some vars for audio sync
    m_src_target = std::max((int)config.src_buffer_target, m_buffer_size);

    // Buffer to inject silence when needed
    m_silence_buf = std::vector<uint8_t>((m_src_target * m_input_spec.freq / m_device_spec.freq) * 2, 0);

    // timestamps for audio sync
    m_block_until_time = m_last_cb_time = std::chrono::steady_clock::now();
    // setup a callback to track when HW requests samples from us
    if (!SDL_SetAudioStreamGetCallback(
            m_stream,
            [](void *userdata, SDL_AudioStream *stream, int additional_amount, int /*total_amount*/) {
                auto *self = (SDLBackend *)userdata;

                if (additional_amount > 0)
                {
                    // If we don't have enough sound data queued up, just inject silence
                    if (additional_amount > self->m_silence_buf.size())
                    {
                        self->m_silence_buf.resize(additional_amount);
                    }
                    SDL_PutAudioStreamData(stream, self->m_silence_buf.data(), (int)self->m_silence_buf.size());
                }
                self->m_last_cb_time = std::chrono::steady_clock::now();
            },
            this))
    {
        throw std::runtime_error(SDL_GetError());
    }

    // update the live settings
    update_cfg_live();
}

SDLBackend::~SDLBackend()
{
    SDL_DestroyAudioStream(m_stream);
    SDL_CloseAudioDevice(m_device_id);
}

void SDLBackend::merge_cfg_live(const Config &config2)
{
    m_config.volume_pct = config2.volume_pct;
    update_cfg_live();
}

void SDLBackend::set_sample_rate(uint32_t sample_rate)
{
    m_input_spec.freq = (int)sample_rate;
    SDL_SetAudioStreamFormat(m_stream, &m_input_spec, nullptr);
}

void SDLBackend::push_samples(void *src, size_t len)
{
    // if we are waiting for audio to catch up, just ignore these samples
    if (std::chrono::steady_clock::now() < m_block_until_time.load()) return;
    // words are stored in DRAM in native order; big-endian pairs of samples will be swapped
    if (m_config.swap_channels ^ (std::endian::native == std::endian::little)) swap_channels(src, len);
    SDL_PutAudioStreamData(m_stream, src, (int)len);
}

void SDLBackend::sync_audio()
{
    constexpr size_t time_tolerance_ms = 30;
    namespace chr = std::chrono;
    using clock_frac = std::chrono::steady_clock::period;

    // determine how many frames we expect by the next audio callback, and also, how many frames we want to have
    size_t expected_frames = estimate_dst_frames_at_next_cb();
    size_t max_target_frames = m_src_target + ((size_t)m_device_spec.freq * time_tolerance_ms / 1000);

    if (expected_frames >= max_target_frames)
    {
        // figure out how long we need to delay the core.
        intmax_t wait_clock_period =
            ((expected_frames - m_src_target) * clock_frac::den) / ((intmax_t)m_device_spec.freq * clock_frac::num);
        auto wait_duration = chr::steady_clock::duration(wait_clock_period);

        set_paused(false);

        if (m_config.sync_audio)
        {
            // throttle the core here to sync up with the audio.
            std::this_thread::sleep_for(wait_duration);
        }
        else
        {
            // block audio until the wait time is up
            m_block_until_time = chr::steady_clock::now() + wait_duration;
        }
    }
    else
    {
        // pause if we don't have enough audio.
        bool need_more_frames = expected_frames < m_buffer_size;
        set_paused(need_more_frames);
    }
}

void SDLBackend::update_cfg_live()
{
    SDL_SetAudioStreamGain(m_stream, ((float)m_config.volume_pct) / 100.0f);
}

size_t SDLBackend::estimate_dst_frames_at_next_cb()
{
    namespace chr = std::chrono;
    using clock_frac = std::chrono::steady_clock::period;

    auto now = chr::steady_clock::now();

    // find the current number of available output frames
    uint32_t bytes_per_frame = SDL_AUDIO_BYTESIZE(m_device_spec.format) * m_device_spec.channels;
    int dst_bytes = SDL_GetAudioStreamAvailable(m_stream);
    if (dst_bytes < 0) throw std::runtime_error(SDL_GetError());
    uint32_t dst_frames = dst_bytes / bytes_per_frame;

    // assume that our audio buffer is filled fast enough to have a full buffer by the next call.
    // we can use this to estimate when the next call should be.
    intmax_t time_to_next_call =
        ((intmax_t)m_buffer_size * clock_frac::den) / ((intmax_t)m_device_spec.freq * clock_frac::num);
    auto predicted_next_cb_time = m_last_cb_time.load() + chr::steady_clock::duration(time_to_next_call);

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
