--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Locks the hotkeys and ensures that atkey is still called for key press (not text type) keyboard events afterwards.
-- Start the script and then:
-- 1. Press a key.
-- 2. If you see "Test passed!" in the console, the test is successful. If not, the test has failed.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

action.lock_hotkeys(true)

emu.atkey(function(args)
    if args.keycode then
        print("Test passed!")
    end
end)

emu.atstop(function()
    action.lock_hotkeys(false)
end)
