--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Draws the peppers test image with different tints.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local img = d2d.load_image(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\peppers.png')

emu.atdrawd2d(function()
    local w = d2d.get_image_info(img).width
    local h = d2d.get_image_info(img).height

    d2d.clear(0, 0, 0, 0)
    d2d.draw_image2({
        identifier = img,
        destx1 = 0,
        desty1 = 0,
    })
    d2d.draw_image2({
        identifier = img,
        destx1 = w,
        desty1 = 0,
        color = { r = 1, g = 0, b = 0, a = 1 },
    })
    d2d.draw_image2({
        identifier = img,
        destx1 = w * 2,
        desty1 = 0,
        color = { r = 0, g = 1, b = 0, a = 1 },
    })
    d2d.draw_image2({
        identifier = img,
        destx1 = w * 3,
        desty1 = 0,
        color = { r = 0, g = 0, b = 1, a = 1 },
    })
    d2d.draw_image2({
        identifier = img,
        destx1 = 0,
        desty1 = h,
        color = { r = 1, g = 1, b = 1, a = 0.5 },
    })
    d2d.draw_image2({
        identifier = img,
        destx1 = w,
        desty1 = h,
        color = { r = 0, g = 0, b = 0, a = 1 }, -- completely black
    })
end)

emu.atstop(function()
    d2d.free_image(img)
end)
