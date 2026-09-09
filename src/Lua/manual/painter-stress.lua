--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Painter API stress test. Press Q/E to switch between the three pages.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local WIDTH = 800
local HEIGHT = 600
local page = 1
local frame = 0
local last_paint_time
local frame_time_ms = 0

local function color(r, g, b, a)
    return { r = r, g = g, b = b, a = a }
end

local function rect(x, y, width, height)
    return { x = x, y = y, width = width, height = height }
end

local background_color = color(0.025, 0.035, 0.055)
local background = painter.brush(background_color)
local white = painter.brush(color(0.92, 0.95, 1.0))
local faint = painter.brush(color(0.50, 0.58, 0.70))
local palette = {
    painter.brush(color(0.95, 0.20, 0.20, 0.32)),
    painter.brush(color(0.20, 0.85, 0.35, 0.32)),
    painter.brush(color(0.20, 0.45, 1.00, 0.32)),
    painter.brush(color(0.95, 0.75, 0.15, 0.32)),
    painter.brush(color(0.75, 0.25, 0.95, 0.32)),
    painter.brush(color(0.10, 0.85, 0.85, 0.32)),
}
local text_style = painter.text_style({ size = 18 })

local small_style = painter.text_style({ size = 13 })

local function draw_header(p)
    p:text(string.format("Frame time: %.2f ms", frame_time_ms), rect(20, 16, 220, 20),
        small_style, white)
    p:text("Page " .. page .. "/2    Q: previous    E: next", rect(20, 570, 760, 20),
        small_style, faint)
end

local function draw_primitives(p)
    p:clear(background_color)
    draw_header(p)

    for i = 1, 140 do
        local x = 70 + ((i * 37) % 660)
        local y = 95 + ((i * 61) % 410)
        local w = 35 + ((i * 19) % 150)
        local h = 25 + ((i * 13) % 115)
        local brush = palette[((i - 1) % #palette) + 1]
        local stroke = { width = 1 + (i % 4), cap = "round", join = "round" }

        p:fill_rect(rect(x, y, w, h), brush)
        p:stroke_rect(rect(x + 8, y + 6, w, h), brush, stroke)
        p:fill_round_rect(rect(x - 12, y + 10, w * 0.8, h * 0.7), 10, brush)
        p:stroke_round_rect(rect(x + 12, y - 8, w * 0.7, h * 0.8), 8, brush, stroke)
        p:fill_ellipse(rect(x - 20, y - 14, w * 0.9, h * 0.8), brush)
        p:stroke_ellipse(rect(x + 18, y + 8, w * 0.65, h * 0.65), brush, stroke)
        p:fill_circle(x + w * 0.5, y + h * 0.5, 10 + (i % 25), brush)
        p:stroke_circle(x + w * 0.4, y + h * 0.6, 14 + (i % 18), brush, stroke)
        p:line(x - 25, y + h + 15, x + w + 25, y - 15, brush, stroke)
        p:polyline({ x - 10, y + h, x + w * 0.3, y - 12,
            x + w * 0.7, y + h + 12, x + w + 20, y + 4 }, brush, stroke)
        p:fill_polygon({ x, y + h * 0.5, x + w * 0.45, y - 18,
            x + w, y + h * 0.25, x + w * 0.65, y + h + 18 }, brush)
        p:stroke_polygon({ x - 8, y + h * 0.5, x + w * 0.45, y - 18,
            x + w + 8, y + h * 0.5, x + w * 0.45, y + h + 18 }, brush, stroke)
    end
end

local function grid_cell(i, x_origin)
    local cell = (i - 1) % 50
    local column = cell % 5
    local row = math.floor(cell / 5)
    return rect(x_origin + column * 79, 55 + row * 50, 75, 42)
end

local function draw_text(p)
    p:clear(background_color)
    draw_header(p)

    for i = 1, 1000 do
        local brush = palette[((i + frame) % #palette) + 1]
        local text = string.format("value %05d / item %03d", math.random(10000, 20000), i)
        p:text(text, grid_cell(i, 10), text_style, brush,
            { overflow = "visible", wrap = "none", clip = false })
    end

    local message = "This is intentionally unchanged painter text. " ..
        "It should exercise caching of immutable layout and glyph data."
    for i = 1, 1000 do
        p:text(message, grid_cell(i, 405), text_style, palette[(i % #palette) + 1],
            { overflow = "visible", wrap = "none", clip = false })
    end
end

emu.atkey(function(args)
    if not args.pressed or args["repeat"] then
        return
    end
    if args.keycode == Mupen.keycode.SDLK_Q then
        page = (page - 2) % 2 + 1
    elseif args.keycode == Mupen.keycode.SDLK_E then
        page = page % 2 + 1
    end
end)

emu.atpaint(function(p)
    local paint_time = os.clock()
    if last_paint_time then
        frame_time_ms = (paint_time - last_paint_time) * 1000
    end
    last_paint_time = paint_time
    frame = frame + 1
    if page == 1 then
        draw_primitives(p)
    else
        draw_text(p)
    end
end)
