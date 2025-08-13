--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Demonstrates the usage of the action API.

local display_name = ""

local SAY_HELLO_WORLD_ACTION = "Action API Demo > Print 'Hello World!'"
local CHANGE_NAME_ACTION = "Action API Demo > Change Name of This Action..."
local CHANGE_HOTKEY_ACTION = "Action API Demo > Change Hotkey..."
local RANDOM_ITEM_ACTION = "Action API Demo > Click Child to Remove It > Item %d"

assert(action.add({
    path = SAY_HELLO_WORLD_ACTION,
    on_press = function()
        print("Hello World!")
    end
}))

assert(action.add({
    path = CHANGE_NAME_ACTION,
    on_press = function()
        local str = input.prompt("Action display name",
            action.get_display_name(CHANGE_NAME_ACTION))
        if str then
            display_name = str
            action.notify_display_name_changed(CHANGE_NAME_ACTION)
        end
    end,
    get_display_name = function()
        print(os.clock())
        return display_name
    end,
}))

assert(action.add({
    path = CHANGE_HOTKEY_ACTION,
    on_press = function()
        local hotkey = hotkey.prompt("Change the hotkey of that other action")
        if hotkey then
            assert(action.associate_hotkey(CHANGE_NAME_ACTION, hotkey, true))
        end
    end
}))

for i = 1, 10, 1 do
    local path = string.format(RANDOM_ITEM_ACTION, i)
    assert(action.add({
        path = path,
        on_press = function()
            local removed_items = action.remove(path)
            print("removed items", removed_items)
        end
    }))
end

assert(action.associate_hotkey(CHANGE_NAME_ACTION, {
    key = string.byte("U"),
    ctrl = true,
}, false))
