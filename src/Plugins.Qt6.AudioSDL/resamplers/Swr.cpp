#include "Swr.hpp"
#include "../Main.hpp"

#include <cassert>
#include <cstdio>
#include <libavutil/mathematics.h>
#include <span>
#include <system_error>
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

void SwrResampler::prepare(uint32_t src_rate, uint32_t dst_rate, bool swap_channels)
{
    // set sample rates, reinit the context
    av_opt_set_int(m_ctx.get(), "in_sample_rate", src_rate, 0);
    av_opt_set_int(m_ctx.get(), "out_sample_rate", dst_rate, 0);

    swr_init(m_ctx.get());
}

bool SwrResampler::push_samples(std::span<const int16_t> samples)
{
    assert(samples.size() % 2 == 0);
    int result;

    uint8_t *ptr = (uint8_t *)samples.data();
    size_t nframes = samples.size() / 2;

    if ((result = swr_convert(m_ctx.get(), nullptr, 0, &ptr, nframes)) < 0)
    {
        throw std::system_error(result, AVErrorCategory::instance());
    }

    return true;
}

uint64_t SwrResampler::buffer_len_out()
{
    int64_t out_sample_rate = 0;
    av_opt_get_int(m_ctx.get(), "out_sample_rate", 0, &out_sample_rate);
    return swr_get_delay(m_ctx.get(), out_sample_rate);
}

bool SwrResampler::pull_samples(std::span<int16_t> samples)
{
    assert(samples.size() % 2 == 0);

    uint8_t *ptr = (uint8_t *)samples.data();
    size_t nframes = samples.size() / 2;

    if (swr_get_out_samples(m_ctx.get(), 0) < nframes)
    {
        return false;
    }

    int result;
    if ((result = swr_convert(m_ctx.get(), &ptr, nframes, nullptr, 0)) < 0) {
        throw std::system_error(result, AVErrorCategory::instance());
    }

    return true;
}

} // namespace AudioSDL