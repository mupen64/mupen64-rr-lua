/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ostream>
namespace SDLAudio
{
struct Config
{
    uint32_t default_sample_rate = 33600;
    uint32_t src_buffer_target = 8192;
    bool swap_channels = false;
    bool sync_audio = false;
    uint8_t volume_pct = 100;

    void write_to(std::ostream &out) const;

    void read_from(std::istream &in);
};
} // namespace SDLAudio
