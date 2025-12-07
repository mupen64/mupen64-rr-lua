#ifndef AUDIOSDL_SDL_BACKEND_HPP_INCLUDED
#define AUDIOSDL_SDL_BACKEND_HPP_INCLUDED

#include "Config.hpp"
#include "BufferUtils.hpp"
#include "Resamplers.hpp"
#include <SDL_audio.h>
#include <chrono>
#include <memory>

namespace AudioSDL
{
class SDLBackend final
{
  public:
    SDLBackend(Config &&cfg = Config{});

    ~SDLBackend();

    void set_sample_rate(uint32_t sample_rate);

    void push_samples(const void *src, size_t len);

    void sync_audio();

    void sdl_callback(unsigned char *data, int len);

  private:
    void reinit_stream();
    size_t estimate_dst_frames_at_next_cb();
    void set_paused(bool paused);

    Config m_config;

    uint32_t m_src_rate;
    uint32_t m_src_target;

    util::buffer<uint16_t> m_src_buffer = {};
    std::unique_ptr<IResampler_Old> m_resampler;
    bool m_paused = true;
    bool m_error = false;
    std::chrono::steady_clock::time_point m_last_cb_time;

    SDL_AudioDeviceID m_audio_device = 0;
    SDL_AudioSpec m_audio_spec = {};
};
} // namespace AudioSDL

#endif