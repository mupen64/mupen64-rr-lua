--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- # Summary
--
-- When `movie.stop` is called from a callback that is invoked by the emu thread
--
-- # Repro
--
-- 1. Load the [attached Lua script](https://github.com/user-attachments/files/20634991/336.txt) until the deadlock happens
--
-- # Expected behavior
--
-- The deadlock shouldn't happen.
--
-- # Technical Side
--
-- In a wider sense, this pertains to doing VCR work in the `input` core callback.
--

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local function evil()
    for i = 1, 100, 1 do
        emu.samplecount()
        emu.inputcount()
        emu.framecount()
        movie.get_seek_completion()
    end
end

emu.atinput(evil)
emu.atvi(evil)
emu.atdrawd2d(evil)
