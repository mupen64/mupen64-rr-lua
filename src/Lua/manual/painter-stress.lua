--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Painter API stress test.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local ROOT <const> = debug.getinfo(1).source:sub(2):gsub("\\[^\\]+$", "")
local WIDTH <const> = 800
local HEIGHT <const> = 600

local frame = 0
local last_paint_time
local frame_time_ms = 0

local function color(r, g, b, a)
    return { r = r, g = g, b = b, a = a }
end

local function rect(x, y, width, height)
    return { x = x, y = y, width = width, height = height }
end

local BACKGROUND_COLOR <const> = color(0.025, 0.035, 0.055)
local BACKGROUND <const> = painter.brush(BACKGROUND_COLOR)
local WHITE <const> = painter.brush(color(0.92, 0.95, 1.0))
local NINESLICED <const> = assert(painter.load_image(ROOT .. '\\..\\ninesliced.png'))
local NINESLICE_OPTIONS <const> = {
    source = rect(0, 0, 32, 32),
    center = rect(15, 15, 2, 2),
    sampling = "nearest",
}
local PALETTE <const> = {
    painter.brush(color(0.95, 0.20, 0.20, 0.32)),
    painter.brush(color(0.20, 0.85, 0.35, 0.32)),
    painter.brush(color(0.20, 0.45, 1.00, 0.32)),
    painter.brush(color(0.95, 0.75, 0.15, 0.32)),
    painter.brush(color(0.75, 0.25, 0.95, 0.32)),
    painter.brush(color(0.10, 0.85, 0.85, 0.32)),
}
local TEXT_STYLE <const> = painter.text_style({ size = 18 })

local SMALL_STYLE <const> = painter.text_style({ size = 13 })

local MEASURE_IDENTICAL_TEXT <const> = "This is an identical string measured repeatedly."
local MEASURE_DIFFERENT_TEXT <const> = {}
for i = 1, 1000 do
    MEASURE_DIFFERENT_TEXT[i] = string.format("This is different string number %04d.", i)
end

local TEXT_OPTIONS <const> = { overflow = "visible", wrap = "none", clip = false }
local TEXT_DIFFERENT <const> = {}
local TEXT_CELLS_LEFT <const> = {}
local TEXT_CELLS_RIGHT <const> = {}
for i = 1, 1000 do
    TEXT_DIFFERENT[i] = string.format("value %05d / item %03d", 10000 + i, i)
    local cell = (i - 1) % 50
    local column = cell % 5
    local row = math.floor(cell / 5)
    TEXT_CELLS_LEFT[i] = rect(10 + column * 79, 55 + row * 50, 75, 42)
    TEXT_CELLS_RIGHT[i] = rect(405 + column * 79, 55 + row * 50, 75, 42)
end
local TEXT_IDENTICAL <const> = "This is intentionally unchanged painter text. " ..
    "It should exercise caching of immutable layout and glyph data."

local PRIMITIVE_CASES <const> = {}
for i = 1, 140 do
    local x = 70 + ((i * 37) % 660)
    local y = 95 + ((i * 61) % 410)
    local w = 35 + ((i * 19) % 150)
    local h = 25 + ((i * 13) % 115)
    PRIMITIVE_CASES[i] = {
        brush = PALETTE[((i - 1) % #PALETTE) + 1],
        stroke = { width = 1 + (i % 4), cap = "round", join = "round" },
        fill_rect = rect(x, y, w, h),
        stroke_rect = rect(x + 8, y + 6, w, h),
        fill_round_rect = rect(x - 12, y + 10, w * 0.8, h * 0.7),
        stroke_round_rect = rect(x + 12, y - 8, w * 0.7, h * 0.8),
        fill_ellipse = rect(x - 20, y - 14, w * 0.9, h * 0.8),
        stroke_ellipse = rect(x + 18, y + 8, w * 0.65, h * 0.65),
        fill_polygon = { x, y + h * 0.5, x + w * 0.45, y - 18,
            x + w, y + h * 0.25, x + w * 0.65, y + h + 18 },
        stroke_polygon = { x - 8, y + h * 0.5, x + w * 0.45, y - 18,
            x + w + 8, y + h * 0.5, x + w * 0.45, y + h + 18 },
        polyline = { x - 10, y + h, x + w * 0.3, y - 12,
            x + w * 0.7, y + h + 12, x + w + 20, y + 4 },
        circle = { x + w * 0.5, y + h * 0.5, 10 + (i % 25) },
        stroke_circle = { x + w * 0.4, y + h * 0.6, 14 + (i % 18) },
        line = { x - 25, y + h + 15, x + w + 25, y - 15 },
    }
end

local IMAGE_CELLS <const> = {}
for i = 1, 1000 do
    local cell = (i - 1) % 100
    local column = cell % 10
    local row = math.floor(cell / 10)
    IMAGE_CELLS[i] = rect(10 + column * 79, 55 + row * 50, 75, 42)
end

local HEADER_FRAME_RECT <const> = rect(20, 16, 220, 20)
local MEASURE_TITLE_RECT <const> = rect(40, 80, 720, 35)
local MEASURE_IDENTICAL_TITLE_RECT <const> = rect(55, 190, 300, 28)
local MEASURE_IDENTICAL_TIME_RECT <const> = rect(75, 235, 300, 25)
local MEASURE_IDENTICAL_LINES_RECT <const> = rect(75, 295, 300, 25)
local MEASURE_DIFFERENT_TITLE_RECT <const> = rect(425, 190, 300, 28)
local MEASURE_DIFFERENT_TIME_RECT <const> = rect(445, 235, 300, 25)
local MEASURE_DIFFERENT_LINES_RECT <const> = rect(445, 295, 300, 25)
local function draw_header(p)
    p:text(string.format("Frame time: %.2f ms", frame_time_ms), HEADER_FRAME_RECT,
        SMALL_STYLE, WHITE)
end

local function draw_primitives(p)
    p:clear(BACKGROUND_COLOR)
    draw_header(p)

    for i = 1, 140 do
        local primitive = PRIMITIVE_CASES[i]
        local brush = primitive.brush
        local stroke = primitive.stroke
        p:fill_rect(primitive.fill_rect, brush)
        p:stroke_rect(primitive.stroke_rect, brush, stroke)
        p:fill_round_rect(primitive.fill_round_rect, 10, brush)
        p:stroke_round_rect(primitive.stroke_round_rect, 8, brush, stroke)
        p:fill_ellipse(primitive.fill_ellipse, brush)
        p:stroke_ellipse(primitive.stroke_ellipse, brush, stroke)
        p:fill_circle(primitive.circle[1], primitive.circle[2], primitive.circle[3], brush)
        p:stroke_circle(primitive.stroke_circle[1], primitive.stroke_circle[2],
            primitive.stroke_circle[3], brush, stroke)
        p:line(primitive.line[1], primitive.line[2], primitive.line[3], primitive.line[4],
            brush, stroke)
        p:polyline(primitive.polyline, brush, stroke)
        p:fill_polygon(primitive.fill_polygon, brush)
        p:stroke_polygon(primitive.stroke_polygon, brush, stroke)
    end
end

local function draw_text(p)
[((i + frame) % #PALETTE) + 1]
        p:text(TEXT_DIFFERENT[i], TEXT_CELLS_LEFT[i], TEXT_STYLE, brush, TEXT_OPTIONS)
    end

    for i = 1, 1000 do
        p:text(TEXT_IDENTICAL, TEXT_CELLS_RIGHT[i], TEXT_STYLE,
            PALETTE[(i % #PALETTE) + 1], TEXT_OPTIONS)
    end
end

local function draw_measure_text(p)
 = 0
    local identical_start = os.clock()
    for _ = 1, 1000 do
        local metrics = painter.measure_text(MEASURE_IDENTICAL_TEXT, TEXT_STYLE)
        identical_lines = identical_lines + metrics.line_count
    end
    local identical_time_ms = (os.clock() - identical_start) * 1000

    local different_lines = 0
    local different_start = os.clock()
    for i = 1, 1000 do
        local metrics = painter.measure_text(MEASURE_DIFFERENT_TEXT[i], TEXT_STYLE)
        different_lines = different_lines + metrics.line_count
    end
    local different_time_ms = (os.clock() - different_start) * 1000

    p:text("measure_text stress test", MEASURE_TITLE_RECT, TEXT_STYLE, WHITE)

    p:text("1000 identical strings", MEASURE_IDENTICAL_TITLE_RECT, TEXT_STYLE, PALETTE[2])
    p:text(string.format("time: %.2f ms", identical_time_ms), MEASURE_IDENTICAL_TIME_RECT,
        SMALL_STYLE, WHITE)
    p:text(string.format("total lines: %d", identical_lines), MEASURE_IDENTICAL_LINES_RECT,
        SMALL_STYLE, WHITE)

    p:text("1000 different strings", MEASURE_DIFFERENT_TITLE_RECT, TEXT_STYLE, PALETTE[3])
    p:text(string.format("time: %.2f ms", different_time_ms), MEASURE_DIFFERENT_TIME_RECT,
        SMALL_STYLE, WHITE)
    p:text(string.format("total lines: %d", different_lines), MEASURE_DIFFERENT_LINES_RECT,
        SMALL_STYLE, WHITE)
end

local function draw_ninesliced(p)
NINESLICED, IMAGE_CELLS[i], NINESLICE_OPTIONS)
    end
end


emu.atpaint(function(p)
    local paint_time = os.clock()
    if last_paint_time then
        frame_time_ms = (paint_time - last_paint_time) * 1000
    end
    last_paint_time = paint_time
    frame = frame + 1
    draw_primitives(p)
    draw_text(p)
    draw_ninesliced(p)
    draw_measure_text(p)
end)
