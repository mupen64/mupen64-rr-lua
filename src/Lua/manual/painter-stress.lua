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

local function rect(x, y, w, h)
    return { x = x, y = y, w = w, h = h }
end

local function merge(base, extra)
    local out = {}
    for k, v in pairs(base) do
        out[k] = v
    end
    for k, v in pairs(extra) do
        out[k] = v
    end
    return out
end

local BACKGROUND_COLOR <const> = color(0.025, 0.035, 0.055)
local WHITE <const> = color(0.92, 0.95, 1.0)
local NINESLICED <const> = assert(painter.load_image(ROOT .. '\\..\\ninesliced.png'))
local NINESLICE_OPTIONS <const> = {
    source = rect(0, 0, 32, 32),
    center = rect(15, 15, 2, 2),
    sampling = "nearest",
}
local PALETTE <const> = {
    color(0.95, 0.20, 0.20, 0.32),
    color(0.20, 0.85, 0.35, 0.32),
    color(0.20, 0.45, 1.00, 0.32),
    color(0.95, 0.75, 0.15, 0.32),
    color(0.75, 0.25, 0.95, 0.32),
    color(0.10, 0.85, 0.85, 0.32),
}
local TEXT_STYLE <const> = { size = 18 }
local STRING_COUNT <const> = 128

local SMALL_STYLE <const> = { size = 13 }

local MEASURE_IDENTICAL_TEXT <const> = "This is an identical string measured repeatedly."
local MEASURE_DIFFERENT_TEXT <const> = {}
for i = 1, STRING_COUNT do
    MEASURE_DIFFERENT_TEXT[i] = string.format("This is different string number %04d.", i)
end

local TEXT_OPTIONS <const> = { overflow = "visible", wrap = "none", clip = false }
local TEXT_STYLE_WITH_OPTIONS <const> = merge(TEXT_STYLE, TEXT_OPTIONS)
local TEXT_STYLE_WITH_FIT <const> = merge(TEXT_STYLE_WITH_OPTIONS, { fit = true })
local TEXT_DIFFERENT <const> = {}
local TEXT_CELLS_LEFT <const> = {}
local TEXT_CELLS_RIGHT <const> = {}
for i = 1, STRING_COUNT do
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
        color = PALETTE[((i - 1) % #PALETTE) + 1],
        stroke = { width = 1 + (i % 4), cap = "round", join = "round" },
        rect_fill = rect(x, y, w, h),
        rect_stroke = rect(x + 8, y + 6, w, h),
        round_rect_fill = rect(x - 12, y + 10, w * 0.8, h * 0.7),
        round_rect_stroke = rect(x + 12, y - 8, w * 0.7, h * 0.8),
        ellipse_fill = rect(x - 20, y - 14, w * 0.9, h * 0.8),
        ellipse_stroke = rect(x + 18, y + 8, w * 0.65, h * 0.65),
        polygon_fill = { x, y + h * 0.5, x + w * 0.45, y - 18,
            x + w, y + h * 0.25, x + w * 0.65, y + h + 18 },
        polygon_stroke = { x - 8, y + h * 0.5, x + w * 0.45, y - 18,
            x + w + 8, y + h * 0.5, x + w * 0.45, y + h + 18 },
        polyline = { x - 10, y + h, x + w * 0.3, y - 12,
            x + w * 0.7, y + h + 12, x + w + 20, y + 4 },
        circle = { x + w * 0.5, y + h * 0.5, 10 + (i % 25) },
        circle_stroke = { x + w * 0.4, y + h * 0.6, 14 + (i % 18) },
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

local ELLIPSE_KAPPA <const> = 0.5522847498307936
local function ellipse_path(p, r)
    local cx <const> = r.x + r.w * 0.5
    local cy <const> = r.y + r.h * 0.5
    local rx <const> = r.w * 0.5
    local ry <const> = r.h * 0.5
    local ox <const> = rx * ELLIPSE_KAPPA
    local oy <const> = ry * ELLIPSE_KAPPA
    p:begin_path()
    p:move_to(cx - rx, cy)
    p:cubic_to(cx - rx, cy - oy, cx - ox, cy - ry, cx, cy - ry)
    p:cubic_to(cx + ox, cy - ry, cx + rx, cy - oy, cx + rx, cy)
    p:cubic_to(cx + rx, cy + oy, cx + ox, cy + ry, cx, cy + ry)
    p:cubic_to(cx - ox, cy + ry, cx - rx, cy + oy, cx - rx, cy)
    p:close_path()
end

local function circle_rect(cx, cy, radius)
    return rect(cx - radius, cy - radius, radius * 2, radius * 2)
end

-- Text is a path primitive, so it is painted by filling a fresh path with the desired color.
local function paint_text(p, value, r, style, fill)
    p:begin_path()
    p:text(value, r, style)
    p:fill(fill)
end

local function draw_header(p)
    paint_text(p, string.format("Frame time: %.2f ms", frame_time_ms), HEADER_FRAME_RECT,
        SMALL_STYLE, WHITE)
end

local function draw_primitives(p)
    p:clear(BACKGROUND_COLOR)
    draw_header(p)

    for i = 1, 140 do
        local primitive = PRIMITIVE_CASES[i]
        local c = primitive.color
        local s = primitive.stroke
        p:begin_path()
        p:rect(primitive.rect_fill)
        p:fill(c)
        p:begin_path()
        p:rect(primitive.rect_stroke)
        p:stroke(c, s)
        p:begin_path()
        p:round_rect(primitive.round_rect_fill, 10)
        p:fill(c)
        p:begin_path()
        p:round_rect(primitive.round_rect_stroke, 8)
        p:stroke(c, s)
        ellipse_path(p, primitive.ellipse_fill)
        p:fill(c)
        ellipse_path(p, primitive.ellipse_stroke)
        p:stroke(c, s)
        p:begin_path()
        p:circle(circle_rect(primitive.circle[1], primitive.circle[2], primitive.circle[3]))
        p:fill(c)
        p:begin_path()
        p:circle(circle_rect(primitive.circle_stroke[1], primitive.circle_stroke[2],
            primitive.circle_stroke[3]))
        p:stroke(c, s)
        p:begin_path()
        p:line(primitive.line[1], primitive.line[2], primitive.line[3], primitive.line[4])
        p:stroke(c, s)
        p:begin_path()
        p:polyline(primitive.polyline)
        p:stroke(c, s)
        p:begin_path()
        p:polygon(primitive.polygon_fill)
        p:fill(c)
        p:begin_path()
        p:polygon(primitive.polygon_stroke)
        p:stroke(c, s)
    end
end

local function draw_text_cells(p)
    for i = 1, STRING_COUNT do
        local c = PALETTE[((i + frame) % #PALETTE) + 1]
        paint_text(p, TEXT_DIFFERENT[i], TEXT_CELLS_LEFT[i], TEXT_STYLE_WITH_OPTIONS, c)
    end

    for i = 1, STRING_COUNT do
        paint_text(p, TEXT_IDENTICAL, TEXT_CELLS_RIGHT[i], TEXT_STYLE_WITH_OPTIONS,
            PALETTE[(i % #PALETTE) + 1])
    end

    for i = 1, STRING_COUNT do
        paint_text(p, TEXT_IDENTICAL, TEXT_CELLS_RIGHT[i], TEXT_STYLE_WITH_FIT,
            PALETTE[((i + 2) % #PALETTE) + 1])
    end
end

local function draw_measure_text(p)
    local identical_lines = 0
    local identical_start = os.clock()
    for _ = 1, STRING_COUNT do
        local metrics = painter.measure_text(MEASURE_IDENTICAL_TEXT, TEXT_STYLE)
        identical_lines = identical_lines + metrics.line_count
    end
    local identical_time_ms = (os.clock() - identical_start) * 1000

    local different_lines = 0
    local different_start = os.clock()
    for i = 1, STRING_COUNT do
        local metrics = painter.measure_text(MEASURE_DIFFERENT_TEXT[i], TEXT_STYLE)
        different_lines = different_lines + metrics.line_count
    end
    local different_time_ms = (os.clock() - different_start) * 1000

    paint_text(p, "measure_text stress test", MEASURE_TITLE_RECT, TEXT_STYLE, WHITE)

    paint_text(p, "128 identical strings", MEASURE_IDENTICAL_TITLE_RECT, TEXT_STYLE, PALETTE[2])
    paint_text(p, string.format("time: %.2f ms", identical_time_ms), MEASURE_IDENTICAL_TIME_RECT,
        SMALL_STYLE, WHITE)
    paint_text(p, string.format("total lines: %d", identical_lines), MEASURE_IDENTICAL_LINES_RECT,
        SMALL_STYLE, WHITE)

    paint_text(p, "128 different strings", MEASURE_DIFFERENT_TITLE_RECT, TEXT_STYLE, PALETTE[3])
    paint_text(p, string.format("time: %.2f ms", different_time_ms), MEASURE_DIFFERENT_TIME_RECT,
        SMALL_STYLE, WHITE)
    paint_text(p, string.format("total lines: %d", different_lines), MEASURE_DIFFERENT_LINES_RECT,
        SMALL_STYLE, WHITE)
end

local function draw_ninesliced(p)
    for i = 1, 500 do
        p:image(NINESLICED, IMAGE_CELLS[i], NINESLICE_OPTIONS)
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
    draw_text_cells(p)
    draw_ninesliced(p)
    draw_measure_text(p)
end)
