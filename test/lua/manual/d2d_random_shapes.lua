--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--


dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local brush = d2d.create_brush(math.random(), math.random(), math.random(), 1)

emu.atdrawd2d(function()
    d2d.clear(0, 0, 0, 0)

    local bounds = wgui.info()
    local x = math.random() * bounds.width
    local y = math.random() * bounds.height
    local w = 50
    local h = 50
    d2d.fill_rectangle(x, y, x + w, y + h, brush)
end)

emu.atstop(function()
    d2d.free_brush(brush)
end)
