/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Encoder.hpp"

class FFmpegEncoder : public Encoder
{
  public:
    std::optional<std::wstring> start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t *image) override;
    bool append_audio(uint8_t *audio, size_t length, uint8_t bitrate) override;
    std::wstring get_desired_extension() const override;
};
