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
local function rect(x, y, w, h)
    return { x = x, y = y, w = w, h = h }
end

local bg = color(0.075, 0.08, 0.10)
local tile_bg = color(0.13, 0.14, 0.16)
local header_bg = color(0.17, 0.18, 0.21)
local edge = color(0.28, 0.29, 0.32)
local text_color = color(0.88, 0.89, 0.90)
local muted = color(0.62, 0.64, 0.67)
local blue = color(0.40, 0.63, 0.72)
local orange = color(0.78, 0.52, 0.30)
local green = color(0.45, 0.68, 0.52)
local purple = color(0.58, 0.50, 0.70)

local colors = {
    text = text_color,
    muted = muted,
    blue = blue,
    orange = orange,
    green = green,
    purple = purple,
    tile_bg = tile_bg,
    header_bg = header_bg,
    edge = edge,
}

local header_style = { size = 13, weight = 700, overflow = "ellipsis" }
local signature_style = { size = 9, wrap = "word", line_height = 1.25 }
local label_style = { size = 10 }
local caption_style = { size = 10, overflow = "ellipsis" }
local body_style = { size = 12 }
local body_center_style = { size = 12, align_x = "center", align_y = "center" }
local body_right_style = { size = 12, align_x = "right" }
local bold_style = { size = 12, weight = 700 }
local italic_style = { size = 12, slant = "italic" }
local decorated_style = { size = 11, underline = true, strikethrough = true, letter_spacing = 0.5 }
local loose_style = { size = 11, line_height = 1.5 }
local wrap_justify_style = { size = 16, weight = 700, align_x = "justify", wrap = "word" }
local wrap_character_style = { size = 16, weight = 700, wrap = "character" }
local overflow_ellipsis_style = { size = 16, overflow = "ellipsis", wrap = "none" }
local overflow_clip_style = { size = 16, overflow = "clip", wrap = "none", clip = true }

local function draw_text(p, value, r, style, fill)
    p:begin_path()
    p:text(value, r, style)
    p:fill(fill)
end

local generated = painter.new_image(116, 76)
generated:paint(function(q)
    q:clear(color(0.10, 0.105, 0.12))
    q:begin_path()
    q:rect(rect(2, 2, 112, 72))
    q:fill(color(0.18, 0.20, 0.22))
    q:begin_path()
    q:rect(rect(16, 18, 32, 40))
    q:fill(color(0.68, 0.48, 0.32))
    q:begin_path()
    q:circle(rect(56, 16, 44, 44))
    q:fill(color(0.36, 0.62, 0.56))
end)

local rt = painter.new_image(48, 48)
rt:paint(function(q)
    q:clear(color(0.16, 0.18, 0.22))
    q:begin_path()
    q:circle(rect(10, 10, 28, 28))
    q:fill(color(0.78, 0.52, 0.30))
    q:begin_path()
    q:circle(rect(4, 4, 40, 40))
    q:stroke(color(0.40, 0.63, 0.72), { width = 2 })
end)

local nested_rt = painter.new_image(56, 56)
nested_rt:paint(function(q)
    q:clear(color(0.10, 0.11, 0.13))
    q:image(rt, rect(4, 4, 24, 24))
    q:image(rt, rect(28, 4, 24, 24), { opacity = 0.7 })
    q:image(rt, rect(16, 28, 24, 24), { tint = color(0.60, 0.80, 0.70) })
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

local TILE_W, TILE_H = 185, 125
local GAP = 14
local MARGIN = 14
local HEADER_W = 132
local SCROLLBAR_W = 12
local WHEEL_STEP = 70
local ROW_PITCH = TILE_H + GAP

local COL_HEADER = MARGIN
local COL_FILL = COL_HEADER + HEADER_W + GAP
local COL_STROKE = COL_FILL + TILE_W + GAP
local COL_COMPLEX_A = COL_STROKE + TILE_W + GAP
local COL_COMPLEX_B = COL_COMPLEX_A + TILE_W + GAP

local function area(x, y)
    return rect(x + 24, y + 18, 137, 64)
end

local function square(x, y)
    local side <const> = 64
    return rect(x + (TILE_W - side) / 2, y + 18, side, side)
end

local STROKE_SIMPLE <const> = { width = 2 }

local rows = {}

local function shape_row(name, signature, path, stroke_a, stroke_b)
    rows[#rows + 1] = {
        name = name,
        signature = signature,
        path = path,
        stroke_a = stroke_a,
        stroke_b = stroke_b,
    }
end

local function custom_row(name, signature, tiles)
    rows[#rows + 1] = { name = name, signature = signature, tiles = tiles }
end

shape_row("rect", "rect(r)", function(q, x, y)
    q:rect(area(x, y))
end, { width = 10, join = "miter", miter_limit = 12 }, { width = 5, dashes = { 14, 8 } })

shape_row("round_rect", "round_rect(r, radius)", function(q, x, y)
    q:round_rect(area(x, y), 14)
end, { width = 9, cap = "round", join = "round" }, { width = 5, dashes = { 4, 7 }, cap = "round" })

shape_row("circle", "circle(r)", function(q, x, y)
    q:circle(square(x, y))
end, { width = 10, cap = "round", dashes = { 2, 9 } }, { width = 6, dashes = { 20, 10 }, dash_offset = 6 })

shape_row("polygon", "polygon(points)", function(q, x, y)
    q:polygon({ x + 24, y + 82, x + 92, y + 18, x + 161, y + 82 })
end, { width = 8, join = "bevel", cap = "square" }, { width = 6, dashes = { 16, 6, 4, 6 } })

shape_row("arc", "arc(x, y, radius, start, end)", function(q, x, y)
    q:arc(x + TILE_W / 2, y + 50, 30, -math.pi / 2, math.pi)
end, { width = 10, cap = "round" }, { width = 6, dashes = { 12, 7 } })

shape_row("polyline", "polyline(points)", function(q, x, y)
    q:polyline({ x + 24, y + 72, x + 58, y + 28, x + 92, y + 72, x + 126, y + 28, x + 161, y + 68 })
end, { width = 9, cap = "square", join = "round" }, { width = 5, dashes = { 18, 10 } })

shape_row("cubic_to", "move_to(m) cubic_to(c1, c2, m)", function(q, x, y)
    q:move_to(x + 24, y + 72)
    q:cubic_to(x + 55, y + 16, x + 130, y + 90, x + 161, y + 34)
end, { width = 9, cap = "round", join = "round" }, { width = 5, dashes = { 14, 8 } })

shape_row("quadratic_to", "move_to(m) quadratic_to(c, m)", function(q, x, y)
    q:move_to(x + 24, y + 72)
    q:quadratic_to(x + 92, y + 6, x + 161, y + 66)
end, { width = 9, cap = "round", join = "round" }, { width = 5, dashes = { 14, 8 } })

shape_row("line", "line(x1, y1, x2, y2)", function(q, x, y, mode)
    if mode == "fill" then
        q:move_to(x + 24, y + 78)
        q:line_to(x + 92, y + 20)
        q:line_to(x + 161, y + 78)
    else
        q:line(x + 24, y + 72, x + 161, y + 28)
    end
end, { width = 12, cap = "round" }, { width = 6, dashes = { 14, 8 } })

custom_row("text", "text(value, r, style)", {
    {
        caption = "weight / slant / decorations",
        draw = function(q, x, y)
            draw_text(q, "bold", rect(x + 15, y + 16, 155, 18), bold_style, colors.text)
            draw_text(q, "italic", rect(x + 15, y + 37, 155, 18), italic_style, colors.blue)
            draw_text(q, "underline + strike", rect(x + 15, y + 58, 155, 20), decorated_style, colors.orange)
        end,
    },
    {
        caption = "align_x / align_y",
        draw = function(q, x, y)
            draw_text(q, "left", rect(x + 15, y + 16, 155, 18), body_style, colors.text)
            draw_text(q, "center", rect(x + 15, y + 37, 155, 18), body_center_style, colors.green)
            draw_text(q, "right", rect(x + 15, y + 58, 155, 18), body_right_style, colors.purple)
        end,
    },
    {
        caption = "wrap",
        draw = function(q, x, y)
            draw_text(q, "word wrapping needs space", rect(x + 15, y + 12, 155, 34), wrap_justify_style, colors.text)
            draw_text(q, "character wrapping hits edge", rect(x + 15, y + 50, 155, 34), wrap_character_style,
                colors.blue)
        end,
    },
    {
        caption = "overflow / measure_text",
        draw = function(q, x, y)
            draw_text(q, "ellipsis: this line is too wide", rect(x + 15, y + 10, 155, 22), overflow_ellipsis_style,
                colors.orange)
            draw_text(q, "clip: this line is too wide", rect(x + 15, y + 36, 155, 22), overflow_clip_style,
                colors.green)
            local metrics = painter.measure_text("measure me", loose_style, { w = 100, wrap = "word" })
            draw_text(q, string.format("%d line(s), %.0f px", metrics.line_count, metrics.w),
                rect(x + 15, y + 66, 155, 18), label_style, colors.muted)
        end,
    },
})

custom_row("fit text", "text(value, r, { fit = true })", {
    {
        caption = "uniform scale to fit",
        draw = function(q, x, y)
            local time = os.clock()
            local width = 48 + (112 * (0.5 + 0.5 * math.sin(time * 2.5)))
            local height = 22 + (62 * (0.5 + 0.5 * math.sin(time * 3.2 + 1.2)))
            local bounds = rect(x + (TILE_W - width) / 2, y + 14 + (68 - height) / 2, width, height)
            q:begin_path()
            q:round_rect(bounds, 6)
            q:fill(color(0.20, 0.24, 0.30, 0.8))
            q:begin_path()
            q:round_rect(bounds, 6)
            q:stroke(colors.blue, { width = 1 })
            draw_text(q, "uniform fit", bounds,
                { size = 28, fit = true, align_x = "center", align_y = "center", wrap = "none" }, colors.text)
        end,
    },
})

custom_row("image", "image(image, destination, options)", {
    {
        caption = "sampling = nearest",
        draw = function(q, x, y)
            q:image(generated, rect(x + 34, y + 16, 116, 76), { sampling = "nearest" })
        end,
    },
    {
        caption = "opacity / tint",
        draw = function(q, x, y)
            q:image(generated, rect(x + 34, y + 16, 116, 76), { opacity = 0.55, tint = color(0.75, 0.55, 0.65) })
        end,
    },
    {
        caption = "source rectangle",
        draw = function(q, x, y)
            q:image(generated, rect(x + 34, y + 16, 116, 76), { source = rect(20, 10, 75, 55), sampling = "linear" })
        end,
    },
    {
        caption = "load_image / decode_image",
        draw = function(q, x, y)
            if loaded then
                q:image(loaded, rect(x + 20, y + 28, 66, 52), { opacity = 0.85 })
            end
            if decoded then
                q:image(decoded, rect(x + 99, y + 28, 66, 52), { tint = color(0.55, 0.70, 0.72) })
            end
        end,
    },
})

custom_row("render target", "PainterImage:paint(callback)", {
    {
        caption = "nine-sliced image",
        draw = function(q, x, y)
            if ninesliced then
                q:image(ninesliced, rect(x + 24, y + 18, 137, 74), {
                    source = rect(0, 0, 32, 32),
                    center = rect(15, 15, 2, 2),
                    sampling = "nearest",
                })
            end
        end,
    },
    {
        caption = "render target",
        draw = function(q, x, y)
            q:image(rt, rect(x + 24, y + 16, 48, 48))
            q:image(rt, rect(x + 84, y + 16, 72, 72), { opacity = 0.85 })
            q:image(rt, rect(x + 24, y + 74, 24, 24), { tint = color(0.60, 0.80, 0.70) })
        end,
    },
    {
        caption = "nested render targets",
        draw = function(q, x, y)
            q:image(nested_rt, rect(x + 24, y + 14, 56, 56))
            q:image(nested_rt, rect(x + 92, y + 14, 72, 72), { opacity = 0.85 })
            q:image(nested_rt, rect(x + 24, y + 72, 28, 28), { tint = color(0.75, 0.65, 0.85) })
        end,
    },
})

custom_row("misc", "state, transforms and colors", {
    {
        caption = "save / clip / restore",
        draw = function(q, x, y)
            q:begin_path()
            q:rect(rect(x + 25, y + 22, 135, 52))
            q:stroke(colors.edge, { width = 1 })
            q:save()
            q:clip(rect(x + 26, y + 23, 133, 50))
            q:begin_path()
            q:polyline({ x + 5, y + 66, x + 48, y + 28, x + 91, y + 66, x + 134, y + 28, x + 177, y + 66 })
            q:stroke(colors.orange, { width = 6, cap = "round", join = "round" })
            q:restore()
        end,
    },
    {
        caption = "translate / rotate / scale",
        draw = function(q, x, y)
            q:save()
            q:translate(x + 92, y + 50)
            q:rotate(math.pi / 8)
            q:scale(1.4, 0.8)
            q:begin_path()
            q:round_rect(rect(-50, -22, 100, 44), 10)
            q:fill(colors.blue)
            q:restore()
        end,
    },
    {
        caption = "hex colors",
        draw = function(q, x, y)
            q:begin_path()
            q:rect(rect(x + 24, y + 18, 64, 32))
            q:fill("#C78550")
            q:begin_path()
            q:rect(rect(x + 97, y + 18, 64, 32))
            q:fill("#5C9E8F80")
            q:begin_path()
            q:round_rect(rect(x + 24, y + 60, 137, 20), 6)
            q:fill("#6BAE84")
            q:begin_path()
            q:rect(rect(x + 24, y + 60, 137, 20))
            q:stroke("#474B52", { width = 1 })
        end,
    },
    {
        caption = "fill then stroke (path reuse)",
        draw = function(q, x, y)
            q:begin_path()
            q:round_rect(rect(x + 30, y + 24, 125, 52), 12)
            q:fill(colors.orange)
            q:stroke(colors.text, { width = 2 })
        end,
    },
})

local CONTENT_H = MARGIN + #rows * TILE_H + (#rows - 1) * GAP + MARGIN
local scroll_y = 0

local function draw_tile(p, x, y, caption, draw)
    p:begin_path()
    p:rect(rect(x, y, TILE_W, TILE_H))
    p:fill(colors.tile_bg)
    p:begin_path()
    p:rect(rect(x, y, TILE_W, TILE_H))
    p:stroke(colors.edge, { width = 1 })
    draw(p, x, y)
    draw_text(p, caption, rect(x + 10, y + TILE_H - 24, TILE_W - 20, 16), caption_style, colors.muted)
end

local function draw_header_tile(p, x, y, name, signature)
    p:begin_path()
    p:rect(rect(x, y, HEADER_W, TILE_H))
    p:fill(colors.header_bg)
    p:begin_path()
    p:rect(rect(x, y, HEADER_W, TILE_H))
    p:stroke(colors.edge, { width = 1 })
    draw_text(p, name, rect(x + 12, y + 20, HEADER_W - 24, 22), header_style, colors.text)
    draw_text(p, signature, rect(x + 12, y + 48, HEADER_W - 24, 60), signature_style, colors.muted)
end

local function describe_stroke(style)
    local parts = {}
    if style.dashes then
        parts[#parts + 1] = "dashes"
    end
    if style.cap then
        parts[#parts + 1] = style.cap .. " cap"
    end
    if style.join then
        parts[#parts + 1] = style.join .. " join"
    end
    parts[#parts + 1] = "w" .. tostring(style.width)
    return table.concat(parts, " ")
end

local function paint_variant(q, row, x, y, mode)
    q:begin_path()
    row.path(q, x, y, mode)
    if mode == "fill" then
        q:fill(colors.orange)
    elseif mode == "stroke" then
        q:stroke(colors.blue, STROKE_SIMPLE)
    elseif mode == "complex_a" then
        q:stroke(colors.green, row.stroke_a)
    else
        q:stroke(colors.purple, row.stroke_b)
    end
end

local function draw_shape_row(p, row, y)
    draw_header_tile(p, COL_HEADER, y, row.name, row.signature)
    draw_tile(p, COL_FILL, y, "fill", function(q, x, ty)
        paint_variant(q, row, x, ty, "fill")
    end)
    draw_tile(p, COL_STROKE, y, "stroke", function(q, x, ty)
        paint_variant(q, row, x, ty, "stroke")
    end)
    draw_tile(p, COL_COMPLEX_A, y, describe_stroke(row.stroke_a), function(q, x, ty)
        paint_variant(q, row, x, ty, "complex_a")
    end)
    draw_tile(p, COL_COMPLEX_B, y, describe_stroke(row.stroke_b), function(q, x, ty)
        paint_variant(q, row, x, ty, "complex_b")
    end)
end

local COLUMNS <const> = { COL_FILL, COL_STROKE, COL_COMPLEX_A, COL_COMPLEX_B }

local function draw_custom_row(p, row, y)
    draw_header_tile(p, COL_HEADER, y, row.name, row.signature)
    for i, tile in ipairs(row.tiles) do
        local x = COLUMNS[i]
        if x then
            draw_tile(p, x, y, tile.caption, tile.draw)
        end
    end
end

local function draw_scrollbar(p, view_w, view_h, max_scroll)
    if max_scroll <= 0 then
        return
    end
    local track_x <const> = view_w - SCROLLBAR_W - 8
    local track_h <const> = view_h - 2 * MARGIN
    if track_h <= 0 then
        return
    end
    local thumb_h <const> = math.max(28, track_h * view_h / CONTENT_H)
    local thumb_y <const> = MARGIN + (track_h - thumb_h) * (scroll_y / max_scroll)

    p:begin_path()
    p:round_rect(rect(track_x, MARGIN, SCROLLBAR_W, track_h), SCROLLBAR_W / 2)
    p:fill(color(1, 1, 1, 0.06))

    p:begin_path()
    p:round_rect(rect(track_x, thumb_y, SCROLLBAR_W, thumb_h), SCROLLBAR_W / 2)
    p:fill(color(1, 1, 1, 0.35))
end

emu.atmouse(function(ev)
    if not ev.y_wheel then
        return
    end
    ev.y_wheel = -ev.y_wheel
    local max_scroll = math.max(0, CONTENT_H - wgui.info().height + 30)
    if max_scroll <= 0 then
        return
    end
    local step = ev.y_wheel > 0 and WHEEL_STEP or -WHEEL_STEP
    scroll_y = math.max(0, math.min(max_scroll, scroll_y + step))
end)

emu.atpaint(function(p)
    local info = wgui.info()
    local view_w, view_h = info.width, info.height
    local max_scroll = math.max(0, CONTENT_H - view_h + 30)
    if scroll_y > max_scroll then
        scroll_y = max_scroll
    end

    p:clear(bg)

    p:save()
    p:translate(0, -scroll_y)
    for i, row in ipairs(rows) do
        local y = MARGIN + (i - 1) * ROW_PITCH
        if row.tiles then
            draw_custom_row(p, row, y)
        else
            draw_shape_row(p, row, y)
        end
    end
    p:restore()

    draw_scrollbar(p, view_w, view_h, max_scroll)
end)
