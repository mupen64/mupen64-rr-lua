#ifndef AUDIOSDL_CONFIG_HPP_INCLUDED
#define AUDIOSDL_CONFIG_HPP_INCLUDED

namespace AudioSDL {
  struct Config {
    uint32_t default_sample_rate = 33600;
    uint32_t src_buffer_size = 16384;
    uint32_t src_buffer_target = 2048;
    uint32_t dst_buffer_size = 1024;
    bool swap_channels = false;
    bool audio_sync = true;
  };
}

#endif