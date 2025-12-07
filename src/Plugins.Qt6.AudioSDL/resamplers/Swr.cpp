#include "Swr.hpp"
extern "C"
{
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
#include <stdexcept>

namespace AudioSDL
{

SwrResampler::SwrResampler() : m_ctx(swr_alloc())
{
    constexpr AVChannelLayout stereo_layout = AV_CHANNEL_LAYOUT_STEREO;

    // Preset channel layout and sample format as those are known
    av_opt_set_chlayout(m_ctx.get(), "in_chlayout", &stereo_layout, 0);
    av_opt_set_chlayout(m_ctx.get(), "out_chlayout", &stereo_layout, 0);
    av_opt_set_sample_fmt(m_ctx.get(), "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_sample_fmt(m_ctx.get(), "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(m_ctx.get(), "filter_type", SWR_FILTER_TYPE_KAISER, 0);
}

void SwrResampler::prepare(uint32_t src_rate, uint32_t dst_rate, size_t dst_size)
{
    // set sample rates, reinit the context
    av_opt_set_int(m_ctx.get(), "in_sample_rate", src_rate, 0);
    av_opt_set_int(m_ctx.get(), "out_sample_rate", dst_rate, 0);
    swr_init(m_ctx.get());
}

void SwrResampler::resample(std::span<uint16_t> src, std::span<uint16_t> dst)
{
    uint8_t *src_data = (uint8_t *)src.data();
    size_t src_frames = src.size() / 2;
    uint8_t *dst_data = (uint8_t *)dst.data();
    size_t dst_frames = dst.size() / 2;

    int res = swr_convert(m_ctx.get(), &dst_data, dst_frames, &src_data, src_frames);
    if (res < 0)
    {
        throw std::runtime_error("Resampling failed");
    }
}

} // namespace AudioSDL