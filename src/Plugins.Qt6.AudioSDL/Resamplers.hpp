#ifndef AUDIOSDL_RESAMPLERS_HPP_INCLUDED
#define AUDIOSDL_RESAMPLERS_HPP_INCLUDED

#include <libavutil/frame.h>
#include <libswresample/swresample.h>
#include <span>

namespace AudioSDL
{

class IResampler {
public:
  virtual ~IResampler() {}

  virtual void prepare(uint32_t src_rate, uint32_t dst_rate, size_t dst_size) = 0;

  virtual void resample(std::span<uint16_t> src, std::span<uint16_t> dst) = 0;
};
} // namespace AudioSDL

#endif