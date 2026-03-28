--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

emu.atupdatescreen(function()
    -- wgui.setcolor("red")
    -- wgui.setbrush("red")
    -- wgui.setpen("red", 1)
    -- wgui.rect(10, 10, 100, 100)
    wgui.fillrecta(0, 0, 100, 100, "red")
end)
