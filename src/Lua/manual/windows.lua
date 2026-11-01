--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

local main = emu.main_window()
assert(main == emu.main_window())
assert(type(emu.atpaint) == 'function' and type(emu.atkey) == 'function' and type(emu.atmouse) == 'function')

local function attach(wnd, name, background)
    local paints, keys, clicks = 0, 0, 0
    wnd:atkey(function(args)
        keys = keys + 1
        print(name, 'key', args.keycode2, args.text)
    end)
    wnd:atmouse(function(args)
        clicks = clicks + 1
        print(name, 'mouse', args.x, args.y, args.button, args.y_wheel)
    end)
    wnd:atpaint(function()
        paints = paints + 1
        local mouse = wnd:mouse_position()
        local p = painter.current()
        p:clear(background)
        p:begin_path()
        p:text(string.format('%s\nPaints: %d\nKeys: %d\nMouse: %d\nPosition: %d, %d', name, paints, keys, clicks, mouse.x, mouse.y),
            { x = 10, y = 10, w = 180, h = 150 }, { size = 14 })
        p:fill({ r = 1, g = 1, b = 1, a = 1 })
    end)
end

attach(main, 'Main', { r = 0, g = 0, b = 0, a = 0 })
for _, anchor in ipairs({ 'left', 'right', 'top', 'bottom' }) do
    local wnd = emu.create_window({ id = 'Window ' .. anchor, anchor = anchor, size = { x = 200, y = 200 } })
    attach(wnd, anchor, { r = 0.1, g = 0.15, b = 0.25, a = 1 })
end
attach(emu.create_window({ id = 'Unanchored', size = { x = 200, y = 200 } }),
    'Unanchored', { r = 0.25, g = 0.15, b = 0.1, a = 1 })

local first_calls, second_calls = 0, 0
local first
first = function()
    first_calls = first_calls + 1
    main:atpaint(first, true)
end
main:atpaint(first)
local second
second = function()
    second_calls = second_calls + 1
    assert(first_calls == 1 and second_calls == 1)
    main:atpaint(second, true)
    print('Callback mutation test passed')
end
main:atpaint(second)
