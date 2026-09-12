--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Provides compatibility shims for old APIs.

-- printx deprecated, forwarded to print
printx = print

-- table.getn deprecated, replaced by # prefix
table.getn = table.getn or function(t)
    return #t
end

-- unpack -> table.unpack
unpack = unpack or table.unpack

-- math.atan2 shim
math.atan2 = math.atan2 or function(y, x)
    if x > 0 then
        return math.atan(y / x)
    elseif x < 0 then
        return math.atan(y / x) + (y >= 0 and math.pi or -math.pi)
    elseif y > 0 then
        return math.pi / 2
    elseif y < 0 then
        return -math.pi / 2
    else
        return 0
    end
end

-- math.pow shim
math.pow = math.pow or function(x, y)
    return x ^ y
end

-- emu.debugview deprecated, forwarded to print
emu.debugview = print

-- emu.setgfx deprecated, no-op
emu.setgfx = function(_) end

-- emu.isreadonly deprecated, forwarded to movie.get_readonly
emu.isreadonly = movie.get_readonly

-- emu.getsystemmetrics is not available anymore due to WinAPI coupling concerns.
emu.getsystemmetrics = function() print('emu.getsystemmetrics has been deprecated') end

-- movie.playmovie deprecated, forwarded to movie.play
movie.playmovie = movie.play

-- movie.stopmovie deprecated, forwarded to movie.stop
movie.stopmovie = movie.stop

-- movie.getmoviefilename deprecated, forwarded to movie.get_filename
movie.getmoviefilename = movie.get_filename

-- movie.isreadonly deprecated, forwarded to movie.get_readonly
movie.isreadonly = movie.get_readonly

-- movie.begin_seek_to is not available anymore due to fundamental unshimmable changes in the seek API.
movie.begin_seek_to = function() print('movie.begin_seek_to has been deprecated, use movie.begin_seek instead') end

-- movie.get_seek_info is not available anymore due to fundamental unshimmable changes in the seek API.
movie.get_seek_info = function() print('movie.get_seek_info has been deprecated, use movie.begin_seek instead') end

-- input.map_virtual_key_ex is not available anymore due to WinAPI coupling concerns.
input.map_virtual_key_ex = function() print('input.map_virtual_key_ex has been deprecated') end

-- memory.recompilenow deprecated, forwarded to memory.recompile
memory.recompilenow = memory.recompile

-- memory.recompilenext deprecated, forwarded to memory.recompile
memory.recompilenext = memory.recompile

---Gets whether fast forward is active.
---@deprecated Use `emu.get_speed_mode` instead.
---@return boolean
function emu.get_ff()
    local mode = emu.get_speed_mode()
    return mode ~= Mupen.CoreSpeedMode.Normal
end

---Sets whether fast forward is active.
---@deprecated Use `emu.set_speed_mode` instead.
---@param fast_forward boolean
function emu.set_ff(fast_forward)
    emu.set_speed_mode(fast_forward and Mupen.CoreSpeedMode.FastForward or Mupen.CoreSpeedMode.Normal)
end

---Saves a savestate to `filename`.
---@param filename string
---@return nil
---@deprecated This function is not guaranteed to succeed successfully or at any specific point in time. Use `savestate.do_file` instead.
function savestate.savefile(filename)
    savestate.do_file(filename, "save", function() end)
end

---Loads a savestate from `filename`.
---@param filename string
---@return nil
---@deprecated This function is not guaranteed to succeed successfully or at any specific point in time. Use `savestate.do_file` instead.
function savestate.loadfile(filename)
    savestate.do_file(filename, "load", function() end)
end

local d2d = {}
local brushes = {}
local images = {}
local next_brush = 1
local next_image = 1
local active_painter

local WHITE = { r = 1, g = 1, b = 1, a = 1 }

local function require_painter()
    if not active_painter then
        error("d2d drawing functions must be called from an emu.atpaint callback", 2)
    end
    return active_painter
end

local function require_brush(handle)
    if handle == 0 then
        return WHITE
    end
    local color = brushes[handle]
    if not color then
        error("invalid d2d brush handle", 3)
    end
    return color
end

local function require_image(handle)
    local image = images[handle]
    if not image then
        error("invalid d2d image identifier", 3)
    end
    return image
end

local function with_path(callback)
    local p = require_painter()
    p:begin_path()
    callback(p)
    return p
end

local atdrawd2d_callbacks = {}
local atdrawd2d_dispatch_registered = false

local function dispatch_atdrawd2d(p)
    active_painter = p
    local callbacks = {}
    for i, callback in ipairs(atdrawd2d_callbacks) do
        callbacks[i] = callback
    end
    local ok, error_message = pcall(function()
        for _, callback in ipairs(callbacks) do
            callback()
        end
    end)
    active_painter = nil
    if not ok then
        error(error_message, 0)
    end
end

---Similar to `emu.atvi`, but for legacy `d2d` and `wgui` drawing commands.
---@param f fun(): nil The function to be called after every VI frame.
---@param unregister boolean? If true, unregister the function `f`.
function emu.atdrawd2d(f, unregister)
    if type(f) ~= "function" then
        error("emu.atdrawd2d expects a function", 2)
    end

    if unregister then
        for i, callback in ipairs(atdrawd2d_callbacks) do
            if callback == f then
                table.remove(atdrawd2d_callbacks, i)
                return
            end
        end
        error("attempt to unregister an unregistered emu.atdrawd2d callback", 2)
    end

    if not atdrawd2d_dispatch_registered then
        emu.atpaint(dispatch_atdrawd2d)
        atdrawd2d_dispatch_registered = true
    end
    table.insert(atdrawd2d_callbacks, f)
end

---@deprecated Use painter colors and Painter:fill instead.
function d2d.create_brush(r, g, b, a)
    local handle = next_brush
    next_brush = next_brush + 1
    brushes[handle] = { r = r, g = g, b = b, a = a }
    return handle
end

---@deprecated Use Lua garbage collection or PainterImage:close instead.
function d2d.free_brush(handle)
    if handle ~= 0 and not brushes[handle] then
        error("invalid d2d brush handle", 2)
    end
    brushes[handle] = nil
end

---@deprecated Use Painter:rect and Painter:fill instead.
function d2d.fill_rectangle(x1, y1, x2, y2, brush)
    with_path(function(p)
        p:rect({ x = x1, y = y1, w = x2 - x1, h = y2 - y1 })
        p:fill(require_brush(brush))
    end)
end

---@deprecated Use Painter:rect and Painter:stroke instead.
function d2d.draw_rectangle(x1, y1, x2, y2, thickness, brush)
    with_path(function(p)
        p:rect({ x = x1, y = y1, w = x2 - x1, h = y2 - y1 })
        p:stroke(require_brush(brush), { width = thickness })
    end)
end

---@deprecated Use Painter:circle and Painter:fill instead.
function d2d.fill_ellipse(x, y, radiusX, radiusY, brush)
    with_path(function(p)
        p:circle({ x = x - radiusX, y = y - radiusY, w = radiusX * 2, h = radiusY * 2 })
        p:fill(require_brush(brush))
    end)
end

---@deprecated Use Painter:circle and Painter:stroke instead.
function d2d.draw_ellipse(x, y, radiusX, radiusY, thickness, brush)
    with_path(function(p)
        p:circle({ x = x - radiusX, y = y - radiusY, w = radiusX * 2, h = radiusY * 2 })
        p:stroke(require_brush(brush), { width = thickness })
    end)
end

---@deprecated Use Painter:line and Painter:stroke instead.
function d2d.draw_line(x1, y1, x2, y2, thickness, brush)
    with_path(function(p)
        p:line(x1, y1, x2, y2)
        p:stroke(require_brush(brush), { width = thickness })
    end)
end

---@deprecated Use Painter:text with PainterTextStyleParams instead.
function d2d.draw_text(x1, y1, x2, y2, text, fontname, fontsize, fontweight, fontstyle, horizalign, vertalign,
    options, brush)
    local slant = fontstyle == 2 and "italic" or fontstyle == 1 and "oblique" or "normal"
    local align_x = horizalign == 1 and "right" or horizalign == 2 and "center" or horizalign == 3 and "justify" or "left"
    local align_y = vertalign == 1 and "bottom" or vertalign == 2 and "center" or "top"
    local clipped = ((options or 0) & 0x2) ~= 0
    with_path(function(p)
        p:text(text, { x = x1, y = y1, w = x2 - x1, h = y2 - y1 }, {
            family = fontname,
            size = fontsize,
            weight = fontweight,
            slant = slant,
            align_x = align_x,
            align_y = align_y,
            overflow = clipped and "clip" or "visible",
            clip = clipped,
        })
        p:fill(require_brush(brush or 0))
    end)
end

---@deprecated Use painter.measure_text instead.
function d2d.get_text_size(text, fontname, fontsize, max_width, max_height)
    local metrics = painter.measure_text(text, { family = fontname, size = fontsize }, {
        w = max_width,
        h = max_height,
        wrap = "word",
    })
    return { width = math.ceil(metrics.w), height = math.ceil(metrics.h) }
end

---@deprecated Use Painter:save and Painter:clip instead.
function d2d.push_clip(x1, y1, x2, y2)
    local p = require_painter()
    p:save()
    p:clip({ x = x1, y = y1, w = x2 - x1, h = y2 - y1 })
end

---@deprecated Use Painter:restore instead.
function d2d.pop_clip()
    require_painter():restore()
end

---@deprecated Use Painter:round_rect and Painter:fill instead.
function d2d.fill_rounded_rectangle(x1, y1, x2, y2, radiusX, radiusY, brush)
    with_path(function(p)
        p:round_rect({ x = x1, y = y1, w = x2 - x1, h = y2 - y1 }, math.min(radiusX, radiusY))
        p:fill(require_brush(brush))
    end)
end

---@deprecated Use Painter:round_rect and Painter:stroke instead.
function d2d.draw_rounded_rectangle(x1, y1, x2, y2, radiusX, radiusY, thickness, brush)
    with_path(function(p)
        p:round_rect({ x = x1, y = y1, w = x2 - x1, h = y2 - y1 }, math.min(radiusX, radiusY))
        p:stroke(require_brush(brush), { width = thickness })
    end)
end

---@deprecated Painter controls text antialiasing automatically; no replacement is needed.
function d2d.set_text_antialias_mode(_)
end

---@deprecated Use painter.load_image instead.
function d2d.load_image(path)
    local image, error_message = painter.load_image(path)
    if not image then
        return nil, error_message
    end
    local identifier = next_image
    next_image = next_image + 1
    images[identifier] = image
    return identifier
end

---@deprecated Use PainterImage:close instead.
function d2d.free_image(identifier)
    local image = require_image(identifier)
    image:close()
    images[identifier] = nil
end

---@deprecated Use PainterImage:paint and painter.new_image instead.
function d2d.draw_to_image(width, height, callback)
    local image = painter.new_image(math.max(1, width), math.max(1, height))
    image:paint(function(p)
        local previous = active_painter
        active_painter = p
        local ok, error_message = pcall(callback)
        active_painter = previous
        if not ok then
            error(error_message, 0)
        end
    end)
    local identifier = next_image
    next_image = next_image + 1
    images[identifier] = image
    return identifier
end

---@deprecated Use Painter:image instead.
function d2d.draw_image2(params)
    local image = require_image(params.identifier)
    local destx2 = params.destx2 or params.destx1 + image.w
    local desty2 = params.desty2 or params.desty1 + image.h
    local srcx1 = params.srcx1 or 0
    local srcy1 = params.srcy1 or 0
    local srcx2 = params.srcx2 or srcx1 + image.w
    local srcy2 = params.srcy2 or srcy1 + image.h
    local options = {
        source = { x = srcx1, y = srcy1, w = srcx2 - srcx1, h = srcy2 - srcy1 },
        sampling = params.interpolation == 0 and "nearest" or "linear",
    }
    if params.color then
        options.tint = { r = params.color.r, g = params.color.g, b = params.color.b, a = 1 }
        options.opacity = params.color.a
    end
    local p = require_painter()
    p:image(image, { x = params.destx1, y = params.desty1, w = destx2 - params.destx1, h = desty2 - params.desty1 }, options)
end

---Draws an image by taking the pixels in the source rectangle of the image, and drawing them to the destination rectangle on the screen.
---@deprecated Use [d2d.draw_image2](lua://d2d.draw_image2) instead.
---@param destx1 integer
---@param desty1 integer
---@param destx2 integer
---@param desty2 integer
---@param srcx1 integer
---@param srcy1 integer
---@param srcx2 integer
---@param srcy2 integer
---@param opacity number
---@param interpolation integer 0: nearest neighbor, 1: linear, -1: don't use.
---@param identifier number
---@return nil
function d2d.draw_image(destx1, desty1, destx2, desty2, srcx1, srcy1, srcx2,
                        srcy2, opacity, interpolation, identifier)
    -- d2d.draw_image2({
    --     identifier = identifier,
    --     destx1 = destx1,
    --     desty1 = desty1,
    --     destx2 = destx2,
    --     desty2 = desty2,
    --     srcx1 = srcx1,
    --     srcy1 = srcy1,
    --     srcx2 = srcx2,
    --     srcy2 = srcy2,
    --     color = opacity == 1 and nil or { r = 1, g = 1, b = 1, a = opacity },
    --     interpolation = interpolation,
    -- })
end

---@deprecated Use PainterImage.w and PainterImage.h instead.
function d2d.get_image_info(identifier)
    local image = require_image(identifier)
    return { width = image.w, height = image.h }
end
_G.d2d = d2d
