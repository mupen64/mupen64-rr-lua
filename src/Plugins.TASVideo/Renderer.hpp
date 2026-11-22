/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "DrawList.hpp"

struct RenderSize
{
    u32 width, height;
};

extern RenderSize render_size;
extern RenderState render_state;
extern BuiltinTAS::Graphics::TextureMemory render_tmem;
extern u32 render_display_list;

inline const gDPTile &render_texture_tile(u32 texture)
{
    return render_state.dp.tiles[render_state.texture_tiles[texture]];
}

inline const gDPTile &render_load_tile()
{
    return render_state.dp.tiles[render_state.load_tile];
}

namespace BuiltinTAS::TASVideo
{
bool renderer_running();
void start_renderer();
void stop_renderer();
void restart_renderer();
void black_out_renderer();
void read_pixels(void *buffer);
} // namespace BuiltinTAS::TASVideo
