--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local root = debug.getinfo(1).source:sub(2):gsub("\\[^\\]+$", "")
local function color(r, g, b, a)
    return { r = r, g = g, b = b, a = a }
end
local function rect(x, y, width, height)
    return { x = x, y = y, width = width, height = height }
end

local bg = color(0.075, 0.08, 0.10)
local tile_bg = color(0.13, 0.14, 0.16)
local edge = color(0.28, 0.29, 0.32)
local text = color(0.88, 0.89, 0.90)
local muted = color(0.62, 0.64, 0.67)
local blue = color(0.40, 0.63, 0.72)
local orange = color(0.78, 0.52, 0.30)
local green = color(0.45, 0.68, 0.52)
local purple = color(0.58, 0.50, 0.70)

local brushes = {
    text = painter.brush(text),
    muted = painter.brush(muted),
    blue = painter.brush(blue),
    orange = painter.brush(orange),
    green = painter.brush(green),
    purple = painter.brush(purple),
    tile_bg = painter.brush(tile_bg),
    edge = painter.brush(edge),
}
local label_style = painter.text_style({ size = 10 })
local body_style = painter.text_style({ size = 12 })
local bold_style = painter.text_style({ size = 12, weight = 700 })
local italic_style = painter.text_style({ size = 12, slant = "italic" })
local decorated_style = painter.text_style({
    size = 11, underline = true, strikethrough = true, letter_spacing = 0.5,
})
local loose_style = painter.text_style({ size = 11, line_height = 1.5 })
local wrap_style = painter.text_style({ size = 16, weight = 700 })
local overflow_style = painter.text_style({ size = 16 })

local generated = painter.new_image(116, 76)
generated:paint(function(q)
    q:clear(color(0.10, 0.105, 0.12))
    q:fill_rect(rect(2, 2, 112, 72), painter.brush(color(0.18, 0.20, 0.22)))
    q:fill_rect(rect(16, 18, 32, 40), painter.brush(color(0.68, 0.48, 0.32)))
    q:fill_circle(78, 38, 22, painter.brush(color(0.36, 0.62, 0.56)))
end)

local source_path = root .. '\\..\\peppers.png'
local loaded = painter.load_image(source_path)
local ninesliced = painter.load_image(root .. '\\..\\ninesliced.png')
local decoded
local file = io.open(source_path, "rb")
if file then
    local data = file:read("*a")
    file:close()
    decoded = painter.decode_image(data)
end

local function tile(p, x, y, title, draw)
    p:fill_rect(rect(x, y, 185, 125), brushes.tile_bg)
    p:stroke_rect(rect(x, y, 185, 125), brushes.edge, { width = 1 })
    draw(p, x, y)
    p:text(title, rect(x + 10, y + 101, 165, 16), label_style, brushes.muted,
        { overflow = "ellipsis" })
end

emu.atpaint(function(p)
    p:clear(bg)

    tile(p, 20, 20, "fill_rect", function(q, x, y)
        q:fill_rect(rect(x + 24, y + 24, 137, 52), brushes.orange)
    end)

    tile(p, 215, 20, "stroke_rect", function(q, x, y)
        q:stroke_rect(rect(x + 24, y + 24, 137, 52), brushes.blue, { width = 2 })
    end)

    tile(p, 410, 20, "fill_round_rect", function(q, x, y)
        q:fill_round_rect(rect(x + 24, y + 24, 137, 52), 10, brushes.green)
    end)

    tile(p, 605, 20, "stroke_round_rect", function(q, x, y)
        q:stroke_round_rect(rect(x + 24, y + 24, 137, 52), 10, brushes.purple, { width = 2 })
    end)

    tile(p, 20, 155, "ellipse / circle", function(q, x, y)
        q:fill_ellipse(rect(x + 20, y + 32, 68, 38), brushes.purple)
        q:stroke_circle(x + 132, y + 51, 24, brushes.orange, { width = 2 })
    end)

    tile(p, 215, 155, "line / polyline", function(q, x, y)
        q:line(x + 22, y + 70, x + 55, y + 35, brushes.blue, { width = 3 })
        q:polyline({ x + 55, y + 70, x + 88, y + 35, x + 121, y + 70,
            x + 154, y + 35 }, brushes.orange, { width = 3 })
    end)

    tile(p, 410, 155, "fill / stroke polygon", function(q, x, y)
        q:fill_polygon({ x + 25, y + 72, x + 92, y + 28, x + 159, y + 72 }, brushes.green)
        q:stroke_polygon({ x + 25, y + 72, x + 92, y + 28, x + 159, y + 72 },
            brushes.text, { width = 2 })
    end)

    tile(p, 605, 155, "push_clip / pop_clip", function(q, x, y)
        q:stroke_rect(rect(x + 25, y + 27, 135, 48), brushes.edge, { width = 1 })
        q:push_clip(rect(x + 26, y + 28, 133, 46))
        q:polyline({ x + 5, y + 68, x + 48, y + 30, x + 91, y + 68,
            x + 134, y + 30, x + 177, y + 68 }, brushes.orange,
            { width = 6, cap = "round", join = "round" })
        q:pop_clip()
    end)

    tile(p, 20, 290, "image", function(q, x, y)
        q:image(generated, rect(x + 34, y + 24, 116, 76), { sampling = "nearest" })
    end)

    tile(p, 215, 290, "opacity / tint", function(q, x, y)
        q:image(generated, rect(x + 34, y + 24, 116, 76),
            { opacity = 0.55, tint = color(0.75, 0.55, 0.65) })
    end)

    tile(p, 410, 290, "source / sampling", function(q, x, y)
        q:image(generated, rect(x + 34, y + 24, 116, 76),
            { source = rect(20, 10, 75, 55), sampling = "linear" })
    end)

    tile(p, 605, 290, "load_image / decode_image", function(q, x, y)
        if loaded then
            q:image(loaded, rect(x + 20, y + 30, 66, 52), { opacity = 0.85 })
        end
        if decoded then
            q:image(decoded, rect(x + 99, y + 30, 66, 52), { tint = color(0.55, 0.70, 0.72) })
        end
    end)

    tile(p, 20, 425, "text: styles", function(q, x, y)
        q:text("bold", rect(x + 15, y + 17, 155, 18), bold_style, brushes.text)
        q:text("italic", rect(x + 15, y + 37, 155, 18), italic_style, brushes.blue)
        q:text("underline + strike", rect(x + 15, y + 57, 155, 18), decorated_style, brushes.orange)
    end)

    tile(p, 215, 425, "text: alignment", function(q, x, y)
        q:text("left", rect(x + 15, y + 16, 155, 18), body_style, brushes.text,
            { align_x = "left" })
        q:text("center", rect(x + 15, y + 37, 155, 18), body_style, brushes.green,
            { align_x = "center", align_y = "center" })
        q:text("right", rect(x + 15, y + 58, 155, 18), body_style, brushes.purple,
            { align_x = "right" })
    end)

    tile(p, 410, 425, "text: wrapping", function(q, x, y)
        q:text("word wrapping needs space", rect(x + 15, y + 12, 155, 34), wrap_style, brushes.text,
            { align_x = "justify", wrap = "word" })
        q:text("character wrapping hits edge", rect(x + 15, y + 50, 155, 34), wrap_style, brushes.blue,
            { wrap = "character" })
    end)

    tile(p, 605, 425, "text: overflow / measure", function(q, x, y)
        q:text("ellipsis: this line is too wide", rect(x + 15, y + 12, 155, 22), overflow_style, brushes.orange,
            { overflow = "ellipsis", wrap = "none" })
        q:text("clip: this line is too wide", rect(x + 15, y + 39, 155, 22), overflow_style, brushes.green,
            { overflow = "clip", wrap = "none", clip = true })
        local metrics = painter.measure_text("measure me", loose_style, { width = 100, wrap = "word" })
        q:text(string.format("%d line(s), %.0f px", metrics.line_count, metrics.width),
            rect(x + 15, y + 68, 155, 18), label_style, brushes.muted)
    end)

    tile(p, 20, 560, "nine-sliced image", function(q, x, y)
        if ninesliced then
            q:image(ninesliced, rect(x + 24, y + 20, 137, 72), {
                source = rect(0, 0, 32, 32),
                center = rect(15, 15, 2, 2),
                sampling = "nearest",
            })
        end
    end)
end)
