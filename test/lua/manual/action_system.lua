--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

--
-- Demonstrates the usage of the action API.
-- Start the script, and ensure that:
-- 1. There is a top-level menu popup titled "Action API Demo"
-- 2. The menu hierarchy matches the action hierarchy
-- 3. Pressing "Print 'Hello World!'" prints "Hello World!" to the console
-- 4. Pressing "Change Name of This Action..." prompts for a new name and changes the name of that action.
-- 5. The hotkey for "Change Name of This Action..." works (default Ctrl+U)
-- 6. Pressing "Change Hotkey..." prompts for a new hotkey and changes the hotkey of "Change Name of This Action..."
-- 7. Pressing "Click Child to Remove It > Item X" removes that action
-- 8. Stopping the script removes the "Action API Demo" menu.
--

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local display_name = ""

local SAY_HELLO_WORLD_ACTION = "Action API Demo > Print 'Hello World!'"
local CHANGE_NAME_ACTION = "Action API Demo > Change Name of This Action..."
local CHANGE_HOTKEY_ACTION = "Action API Demo > Change Hotkey..."
local TOGGLE_HOTKEY_LOCK_ACTION = "Action API Demo > Lock/Unlock Hotkeys"
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

assert(action.add({
    path = TOGGLE_HOTKEY_LOCK_ACTION,
    on_press = function()
        action.lock_hotkeys(not action.get_hotkeys_locked())
        print("Hotkeys are now " .. (action.get_hotkeys_locked() and "locked" or "unlocked"))
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
