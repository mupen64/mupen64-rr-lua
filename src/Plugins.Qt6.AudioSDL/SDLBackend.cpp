#include "SDLBackend.hpp"
#include "Main.hpp"
#include "resamplers/Swr.hpp"
#include <SDL2/SDL.h>
#include <SDL_audio.h>
#include <SDL_timer.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <span>
#include <stdexcept>
#include <thread>

namespace
{
constexpr uint32_t SDL_BACKEND_INIT = SDL_INIT_AUDIO | SDL_INIT_TIMER;

// Safer wrapper for SDL_LockAudioDevice.
struct SDLAudioLock
{

    SDLAudioLock(SDL_AudioDeviceID device = 1) : m_device(device) { SDL_LockAudioDevice(m_device); }

    SDLAudioLock(const SDLAudioLock &) = delete;
    SDLAudioLock &operator=(const SDLAudioLock &) = delete;
    SDLAudioLock(SDLAudioLock &&) = delete;
    SDLAudioLock &operator=(SDLAudioLock &&) = delete;

    ~SDLAudioLock() { SDL_UnlockAudioDevice(m_device); }

    SDL_AudioDeviceID m_device;
};

uint32_t select_dst_rate(uint32_t src_rate)
{
    if (src_rate <= 11025)
        return 11025;
    else if (src_rate <= 22050)
        return 22050;
    else
        return 44100;
}

void sdl_backend_audio_cb(void *udata, unsigned char *data, int len)
{
    ((AudioSDL::SDLBackend *)udata)->sdl_callback(data, len);
}

void u16_swap_copy(uint16_t *dst, uint16_t *src, size_t len)
{
    for (size_t i = 0; i < len; i += 2)
    {
        dst[i] = src[i + 1];
        dst[i + 1] = src[i];
    }
}

} // namespace

namespace AudioSDL
{
SDLBackend::SDLBackend(Config &&cfg)
    : m_src_rate(cfg.default_sample_rate), m_last_cb_time(std::chrono::steady_clock::now()),
      m_resampler(new SwrResampler_Old())
{
    if (SDL_Init(SDL_BACKEND_INIT))
    {
        throw std::runtime_error("SDL init failed");
    }

    reinit_stream();
}

SDLBackend::~SDLBackend()
{

    if (m_audio_device != 0)
    {
        SDL_PauseAudioDevice(m_audio_device, 1);
        SDL_CloseAudioDevice(m_audio_device);
    }
    SDL_QuitSubSystem(SDL_BACKEND_INIT);
}

void SDLBackend::set_paused(bool paused)
{
    if (m_paused != paused) SDL_PauseAudioDevice(m_audio_device, (int)paused);
    m_paused = paused;
}

void SDLBackend::reinit_stream()
{
    m_error = false;

    if (m_audio_device != 0)
    {
        SDL_PauseAudioDevice(m_audio_device, 1);
        SDL_CloseAudioDevice(m_audio_device);
    }

    SDL_AudioSpec desired_spec{
        .freq = (int)select_dst_rate(m_src_rate),
        .format = AUDIO_S16SYS,
        .channels = 2,
        .samples = (uint16_t)m_config.dst_buffer_size,
        .callback = sdl_backend_audio_cb,
        .userdata = this,
    };

    m_audio_device = SDL_OpenAudioDevice(nullptr, false, &desired_spec, &m_audio_spec, 0);
    if (m_audio_device == 0)
    {
        m_audio_spec = {};
        throw std::runtime_error("SDL_OpenAudioDevice failed");
    }

    // SDL2 starts streams paused.
    m_paused = true;

    m_src_target = std::max(m_config.src_buffer_target, (uint32_t)m_audio_spec.samples);
    uint32_t src_size_samples = std::max(m_config.src_buffer_size, m_src_target);

    // configuration values are measured in output samples. Each "sample" actually consists of 2 elements
    // so we multiply by 2, then convert from output rate to input rate.
    size_t src_buffer_size = src_size_samples * 2 * (size_t)m_src_rate / (size_t)m_audio_spec.freq;
    // {
    //     static char log_buf[256];
    //     snprintf(log_buf, sizeof(log_buf), "%zu, %zu, %zu -> %zu", (size_t)src_size_samples, (size_t)m_src_rate,
    //     (size_t)m_audio_spec.freq, src_buffer_size); g_fwd_funcs->log_info(log_buf);
    // }
    m_src_buffer.reserve(src_buffer_size);

    m_resampler->prepare(m_src_rate, (uint32_t)m_audio_spec.freq, m_audio_spec.samples);
}

void SDLBackend::sdl_callback(unsigned char *data, int len_bytes)
{
    // check the time
    m_last_cb_time = std::chrono::steady_clock::now();

    // compute number of needed samples
    size_t new_rate = m_audio_spec.freq;
    size_t old_rate = m_src_rate;
    size_t len_samples = len_bytes / 2;
    size_t len_needed = len_samples * old_rate / new_rate + 4;

    if ((m_src_buffer.size() > 0) && (m_src_buffer.size() >= len_needed))
    {
        // resample
        size_t resample_count = m_resampler->resample(std::span<uint16_t>(m_src_buffer.data(), len_needed),
                                                      std::span<uint16_t>((uint16_t *)data, len_samples));

        if (resample_count * 2 < len_samples) {
            static char printout[256];
            snprintf(printout, sizeof(printout), "%zu < %zu", resample_count * 2, len_samples);
            g_fwd_funcs->log_info(printout);
        }
        // pop resampled bytes
        m_src_buffer.erase(m_src_buffer.begin(), m_src_buffer.begin() + len_needed);
    }
    else
    {
        // buffer underrun!
        g_fwd_funcs->log_warn("hit buffer underrun");
        memset(data, 0, len_bytes);
    }
}

void SDLBackend::set_sample_rate(uint32_t sample_rate)
{
    if (m_error) return;

    m_src_rate = sample_rate;
    reinit_stream();
}

void SDLBackend::push_samples(const void *src, size_t len_bytes)
{
    if (m_error) return;

    // Lock the audio device.
    SDLAudioLock _lock(m_audio_device);

    // ignore any incompletely-filled frames.
    if (len_bytes % 4 != 0)
    {
        // this effectively does len_bytes - (len_bytes % 4).
        len_bytes &= ~0x03;
    }

    size_t len_samples = len_bytes / 2;

    // prepare to copy the necessary samples.
    size_t len_prev = m_src_buffer.size();
    if (len_prev + len_bytes > m_src_buffer.capacity())
    {
        // avoid any potential reallocation.
        return;
    }

    m_src_buffer.resize(len_prev + len_bytes);
    uint16_t *copy_start = m_src_buffer.data() + len_prev;

    // Core RDRAM stores 32-bit words in native byte order.
    // On little-endian systems, this means we have to switch
    // the channels to make things sound right.
    if (m_config.swap_channels ^ (std::endian::native == std::endian::little))
    {
        u16_swap_copy(copy_start, (uint16_t *)src, len_samples);
    }
    else
    {
        memcpy(copy_start, (uint16_t *)src, len_bytes);
    }
}

size_t SDLBackend::estimate_dst_frames_at_next_cb()
{
    namespace chr = std::chrono;
    using clock_frac = std::chrono::steady_clock::period;

    auto now = chr::steady_clock::now();

    // number of frames currently stored
    size_t src_frames = m_src_buffer.size() / 2;

    // compute the current number of available output frames
    size_t dst_frames = (src_frames * (size_t)m_audio_spec.freq) / m_src_rate;

    // assume that our audio buffer is filled fast enough to have a full buffer by the next call.
    // we can use this to estimate when the next call should be.
    intmax_t time_to_next_call =
        ((intmax_t)m_audio_spec.samples * clock_frac::den) / ((intmax_t)m_audio_spec.freq * clock_frac::num);
    auto predicted_next_cb_time = m_last_cb_time + chr::steady_clock::duration(time_to_next_call);

    // if there's still time to go, count in however many samples should be added between now and callback time
    if (now < predicted_next_cb_time)
    {
        dst_frames += ((predicted_next_cb_time - now).count() * m_audio_spec.freq * clock_frac::num) / clock_frac::den;
    }

    return dst_frames;
}

void SDLBackend::sync_audio()
{
    constexpr size_t TIME_TOLERANCE_MS = 10;
    namespace chr = std::chrono;
    using clock_frac = std::chrono::steady_clock::period;

    size_t expected_frames = estimate_dst_frames_at_next_cb();
    size_t max_target_frames = m_src_target + ((size_t)m_audio_spec.freq * TIME_TOLERANCE_MS / 1000);

    // {
    //     static char log_buf[256];
    //     snprintf(log_buf, sizeof(log_buf), "%zu : %zu", expected_frames, max_target_frames);
    //     g_fwd_funcs->log_info(log_buf);
    // }

    if (m_config.audio_sync && (expected_frames >= max_target_frames))
    {
        // figure out how long we need to delay the core.
        intmax_t wait_clock_period =
            ((expected_frames - m_src_target) * clock_frac::den) / ((intmax_t)m_audio_spec.freq * clock_frac::num);
        auto wait_duration = chr::steady_clock::duration(wait_clock_period);

        // If the core is ahead, have it wait here to sync up with the audio.
        set_paused(false);
        std::this_thread::sleep_for(wait_duration);
    }
    else if (expected_frames < m_audio_spec.samples)
    {
        // we won't have enough audio, pause until we do.
        set_paused(true);
    }
    else
    {
        // we have enough audio.
        set_paused(false);
    }
}

} // namespace AudioSDL