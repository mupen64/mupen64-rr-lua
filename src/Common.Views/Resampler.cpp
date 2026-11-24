/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/Resampler.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <speex/speex_resampler.h>
#include <speex/speexdsp_types.h> // NOLINT(misc-include-cleaner)
#include <utility>

void Resampler::Deleter::operator()(SpeexResamplerState_ *state) const
{
    speex_resampler_destroy(state);
}

bool Resampler::process(std::span<const short> input, uint32_t source_rate, uint32_t output_rate, bool swap_channels)
{
    constexpr size_t channels = 2;
    const auto frames = static_cast<spx_uint32_t>(input.size() / channels);
    if (source_rate == output_rate && !m_state)
    {
        m_output.assign(input.begin(), input.first(frames * channels).end());
    }
    else
    {
        if (!m_state)
        {
            m_state.reset(speex_resampler_init(channels, source_rate, output_rate, 6, nullptr));
            if (!m_state) return false;
            m_source_rate = source_rate;
        }
        if (m_source_rate != source_rate)
        {
            if (speex_resampler_set_rate(m_state.get(), source_rate, output_rate) != RESAMPLER_ERR_SUCCESS)
                return false;
            m_source_rate = source_rate;
        }

        const auto capacity = (uint64_t{frames} * output_rate + source_rate - 1) / source_rate + 1;
        spx_uint32_t input_offset = 0;
        size_t output_offset = 0;
        while (input_offset < frames)
        {
            m_output.resize(output_offset + capacity * channels);
            auto consumed = frames - input_offset;
            auto produced = static_cast<spx_uint32_t>(capacity);
            if (speex_resampler_process_interleaved_int(m_state.get(), input.data() + input_offset * channels,
                    &consumed, m_output.data() + output_offset, &produced) != RESAMPLER_ERR_SUCCESS ||
                (consumed == 0 && produced == 0))
                return false;

            input_offset += consumed;
            output_offset += produced * channels;
        }
        m_output.resize(output_offset);
    }

    if (swap_channels)
    {
        for (size_t i = 0; i < m_output.size(); i += channels)
        {
            std::swap(m_output[i], m_output[i + 1]);
        }
    }
    return true;
}
