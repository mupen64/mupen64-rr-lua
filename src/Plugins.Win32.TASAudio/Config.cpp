/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Config.hpp"
#include <iomanip>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <stdexcept>

namespace SDLAudio
{

void Config::read_from(std::istream &in)
{
    nlohmann::json j = nlohmann::json::parse(in);
    if (!j.is_object()) throw std::invalid_argument("invalid config");

    swap_channels = j["swap_channels"];
    sync_audio = j["sync_audio"];
    volume_pct = (uint32_t)j["volume_db"];
}

void Config::write_to(std::ostream &out) const
{
    nlohmann::json j{{"swap_channels", swap_channels}, {"sync_audio", sync_audio}, {"volume_db", (uint32_t)volume_pct}};

    auto old_flags = out.flags();
    out << std::setw(2) << j << '\n';
    out.flags(old_flags);
}

} // namespace SDLAudio