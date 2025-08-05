--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Sets up some actions.

local result = action.add({
    path = "My Cool Lua > Do Something Cool",
    down_callback = function()
        print("Hello World!")
    end
})

assert(result)

local result = action.add({
    path = "My Cool Lua > Dynamically Named Action!",
    down_callback = function()
        print("the displayed name is: " .. action.get_display_name("My Cool Lua > Dynamically Named Action!"))
    end,
    get_real_name = function ()
        return "The time is " .. os.date("%H:%M:%S", os.time())
    end,
})

assert(result)

local result = action.associate_hotkey("My Cool Lua > Do Something Cool", {
    key = string.byte("U"),
    ctrl = true,
})

assert(result)
