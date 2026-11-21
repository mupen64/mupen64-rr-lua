/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

struct SpeexResamplerState_;

class Resampler
{
  public:
    bool process(std::span<const short> input, uint32_t source_rate, uint32_t output_rate, bool swap_channels);
    bool active() const { return static_cast<bool>(m_state); }
    std::span<short> output() { return m_output; }

  private:
    struct Deleter
    {
        void operator()(SpeexResamplerState_ *state) const;
    };

    std::unique_ptr<SpeexResamplerState_, Deleter> m_state;
    uint32_t m_source_rate{};
    std::vector<short> m_output;
};
