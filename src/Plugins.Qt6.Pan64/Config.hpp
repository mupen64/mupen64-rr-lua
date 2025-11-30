#ifndef PAN64_CONFIG_HPP_INCLUDED
#define PAN64_CONFIG_HPP_INCLUDED

namespace Pan64 {
  struct Config {
    uint32_t in_buffer_size;
    uint32_t in_buffer_target;
    uint32_t out_buffer_size;
    
    uint32_t default_sample_rate;
    bool swap_channels;
  };
}

#endif