#include "Resampler.hpp"
#include <samplerate.h>
#include <system_error>

namespace Pan64 {

SrcResampler::SrcResampler(int sampler_type) {
  int error;
  m_state.reset(src_new(sampler_type, 2, &error));
  if (error != 0) {
    throw std::system_error(error, std::generic_category());
  }
}

size_t SrcResampler::resample(std::span<uint8_t> in_data, uint32_t in_rate, std::span<uint8_t> out_data, uint32_t out_rate) {
  
  return 0;
}
}