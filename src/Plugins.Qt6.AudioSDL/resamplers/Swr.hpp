#ifndef AUDIOSDL_RESAMPLERS_SWR_HPP_INCLUDED
#define AUDIOSDL_RESAMPLERS_SWR_HPP_INCLUDED

#include <libavutil/error.h>
#include <string>
#include <system_error>
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

inline void swr_delete(SwrContext *ptr)
{
    swr_free(&ptr);
}

using PSwrContext = util::c_unique_ptr<SwrContext, swr_delete>;

class AVErrorCategory : public std::error_category
{
  public:
    static const AVErrorCategory &instance()
    {
        static AVErrorCategory category;
        return category;
    }

    virtual const char *name() const noexcept { return "FFmpeg"; }

    virtual std::string message(int condition) const { return std::string(av_err2str(condition)); }

  private:
    AVErrorCategory() {}
};

class SwrResampler_Old : public IResampler_Old
{
  public:
    SwrResampler_Old();

    virtual ~SwrResampler_Old() {}

    virtual void prepare(uint32_t src_rate, uint32_t dst_rate, size_t dst_size) override;

    virtual size_t required_input(size_t out_frames) override;

    virtual size_t resample(std::span<uint16_t> src, std::span<uint16_t> dst) override;

  private:
    PSwrContext m_ctx;
};

class SwrResampler : public IResampler
{
  public:
    SwrResampler();

    virtual ~SwrResampler() {}

    virtual void prepare(uint32_t src_rate, uint32_t dst_rate, bool swap_channels) override;

    virtual bool push_samples(std::span<const int16_t> samples) override;

    virtual uint64_t buffer_len_out() override;

    virtual bool pull_samples(std::span<int16_t> samples) override;

  private:
    PSwrContext m_ctx;
};
} // namespace AudioSDL

#endif