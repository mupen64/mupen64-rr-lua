#ifndef AUDIOSDL_RESAMPLERS_SWR_HPP_INCLUDED
#define AUDIOSDL_RESAMPLERS_SWR_HPP_INCLUDED

extern "C"
{
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
#include <span>
}

#include "../BufferUtils.hpp"
#include "../Resamplers.hpp"

namespace AudioSDL
{

using PSwrContext = util::c_unique_ptr<SwrContext, [](SwrContext *ptr) { swr_free(&ptr); }>;

class SwrResampler : public IResampler
{
  public:
    SwrResampler();

    virtual ~SwrResampler() {}

    virtual void prepare(uint32_t src_rate, uint32_t dst_rate, size_t dst_size);

    virtual void resample(std::span<uint16_t> src, std::span<uint16_t> dst);

  private:
    PSwrContext m_ctx;
};
} // namespace AudioSDL

#endif