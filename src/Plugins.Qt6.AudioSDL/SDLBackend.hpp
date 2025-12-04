#ifndef AUDIOSDL_SDL_BACKEND_HPP_INCLUDED
#define AUDIOSDL_SDL_BACKEND_HPP_INCLUDED

#include "Config.hpp"
#include <SDL_audio.h>
#include <optional>
#include <vector>

namespace AudioSDL {
  class SDLBackend final {
  public:
    SDLBackend(Config&& cfg = Config {});

    ~SDLBackend();

    void set_sample_rate(uint32_t sample_rate);

    void push_samples(const void* src, size_t len);

    void sync_audio();

    void sdl_callback(unsigned char* data, int len);
  private:
    void reinit_stream();

    Config m_config;

    uint32_t m_src_rate;
    uint32_t m_src_target;

    std::vector<uint16_t> m_src_buffer = {};
    bool m_paused_sync = true;

    SDL_AudioDeviceID m_audio_device = 0;
    SDL_AudioSpec m_audio_spec = {};
  };
}

#endif