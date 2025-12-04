#include "SDLBackend.hpp"
#include <SDL2/SDL.h>
#include <SDL_audio.h>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace
{
constexpr uint32_t SDL_BACKEND_INIT = SDL_INIT_AUDIO | SDL_INIT_TIMER;

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
} // namespace

namespace AudioSDL
{
SDLBackend::SDLBackend(Config &&cfg) : m_src_rate(cfg.default_sample_rate)
{
    m_paused_sync = true;

    if (SDL_Init(SDL_BACKEND_INIT))
    {
        throw std::runtime_error("SDL init failed");
    }

    reinit_stream();
}

SDLBackend::~SDLBackend()
{
    SDL_QuitSubSystem(SDL_BACKEND_INIT);
}

void SDLBackend::reinit_stream()
{
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
        throw std::runtime_error("SDL_OpenAudioDevice failed");
    }

    m_src_target = std::max(m_config.src_buffer_target, (uint32_t) m_audio_spec.samples);
    uint32_t src_size = std::max(m_config.src_buffer_size, m_src_target);

    m_src_buffer.resize(src_size);
}
} // namespace AudioSDL