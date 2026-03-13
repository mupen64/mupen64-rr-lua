--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--



dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local black_brush = d2d.create_brush(0, 0, 0, 1)

---@type DgfxCommand[]
local cmds = {}

emu.atdrawd2d(function()
    cmds[#cmds + 1] = {
        type = Mupen.DgfxCommandType.CLEAR,
        color = { 0, 0, 0, 0 },
    }

    for x = 0, 100, 1 do
        for y = 0, 100, 1 do
            local x1 = x * 10
            local y1 = y * 10
            cmds[#cmds + 1] = {
                type = Mupen.DgfxCommandType.TEXT,
                color = { 1, 0, 0, 1 },
                x = x1,
                y = y1,
                w = x1 + 1000,
                h = y1 + 1000,
                text = "Hello World",
                font_name = "Arial",
                font_size = 12,
                font_weight = 400,
                font_style = 0,
                horizontal_alignment = 0,
                vertical_alignment = 0,
                options = 0,
            }
        end
    end

    dgfx.enqueue(cmds)
end)

emu.atstop(function()
    d2d.free_brush(black_brush)
end)
