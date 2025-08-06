--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Sets up some actions.

assert(action.add({
    path = "My Cool Lua > Do Something Cool",
    down_callback = function()
        print("Hello World!")
    end
}))

assert(action.add({
    path = "My Cool Lua > Dynamically Named Action! ---",
    down_callback = function()
        print("the displayed name is: " .. action.get_display_name("My Cool Lua > Dynamically Named Action! ---"))
    end,
    get_display_name = function()
        return "The time is " .. os.date("%H:%M:%S", os.time())
    end,
}))

assert(action.associate_hotkey("My Cool Lua > Do Something Cool", {
    key = string.byte("U"),
    ctrl = true,
}, false))

assert(action.add({
    path = "My Cool Lua > Change hotkey of first action",
    down_callback = function()
        local hotkey = hotkey.prompt("Change the hotkey of blah blah something something")
        if hotkey then
            assert(action.associate_hotkey("My Cool Lua > Do Something Cool", hotkey, true))
        end
    end
}))
