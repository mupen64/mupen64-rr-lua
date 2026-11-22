/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Plugin.hpp>

namespace BuiltinTAS::Graphics
{
struct DrawList;

// The renderer consumes the list before returning. Background textures can be read from RDRAM during this call.
using SubmitDrawList = void (*)(const DrawList &);

void reset();
void process_task(M64RRSpec::PluginInit &plugin, SubmitDrawList submit);
} // namespace BuiltinTAS::Graphics

namespace BuiltinTAS::TASRSP
{
void configure_graphics(bool use_builtin_decoder, Graphics::SubmitDrawList submit);
}

namespace BuiltinTAS::TASVideo
{
void submit_draw_list(const Graphics::DrawList &list);
}
