--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Draws the peppers test image with different tints.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local img = assert(painter.load_image(
    debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\peppers.png'))

emu.atpaint(function(p)
    local w = img.width
    local h = img.height

    p:clear({ r = 0, g = 0, b = 0, a = 0 })
    p:image(img, { x = 0, y = 0, width = w, height = h })
    p:image(img, { x = w, y = 0, width = w, height = h }, { tint = { r = 1, g = 0, b = 0 } })
    p:image(img, { x = w * 2, y = 0, width = w, height = h }, { tint = { r = 0, g = 1, b = 0 } })
    p:image(img, { x = w * 3, y = 0, width = w, height = h }, { tint = { r = 0, g = 0, b = 1 } })
    p:image(img, { x = 0, y = h, width = w, height = h }, { opacity = 0.5 })
    p:image(img, { x = w, y = h, width = w, height = h }, { tint = { r = 0, g = 0, b = 0 } })
end)
