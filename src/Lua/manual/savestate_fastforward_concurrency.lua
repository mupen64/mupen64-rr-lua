--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Start the script and ensure there's no error.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

emu.set_ff(true)
emu.pause(true)

local buf = ""
savestate.do_memory("", "save", function(result, data)
    buf = data
end)

local load_requested = false
local load_completed = false

emu.atinput(function()
    if load_requested and not load_completed then
        error("Savestate operation didn't finish before the next frame!")
    end

    load_requested = true
    load_completed = false

    savestate.do_memory(buf, "load", function (result, data)
        load_completed = true
    end)
end)
