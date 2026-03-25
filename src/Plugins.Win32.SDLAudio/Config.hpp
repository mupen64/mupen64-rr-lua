/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace SDLAudio
{
struct Config
{
    uint32_t default_sample_rate = 33600;
    uint32_t src_buffer_size = 16384;
    uint32_t src_buffer_target = 2048;
    uint32_t dst_buffer_size = 1024;
    bool swap_channels = false;
    bool audio_sync = false;
};
} // namespace SDLAudio