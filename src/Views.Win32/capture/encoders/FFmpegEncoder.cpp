/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "FFmpegEncoder.hpp"

std::optional<std::wstring> FFmpegEncoder::start(Params)
{
    return L"FFmpeg encoding is not available in this build.";
}

bool FFmpegEncoder::stop()
{
    return true;
}

bool FFmpegEncoder::append_video(uint8_t *)
{
    return false;
}

bool FFmpegEncoder::append_audio(uint8_t *, size_t, uint8_t)
{
    return false;
}

std::wstring FFmpegEncoder::get_desired_extension() const
{
    return L".mp4";
}
