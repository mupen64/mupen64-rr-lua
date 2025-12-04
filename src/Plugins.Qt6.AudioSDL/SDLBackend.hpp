#ifndef AUDIOSDL_SDL_BACKEND_HPP_INCLUDED
#define AUDIOSDL_SDL_BACKEND_HPP_INCLUDED

#include "Config.hpp"
#include <vector>

namespace AudioSDL {
  class SDLAudioLock {

  };

  class SDLBackend final {
  public:
    SDLBackend(Config cfg = Config {});

    ~SDLBackend();

    void set_sample_rate(uint32_t sample_rate);

    void push_samples(const void* src, size_t len);

    void sync_audio();
  private:
    Config m_config;
    std::vector<uint16_t> m_in_buffer; 
  };
}

#endif