--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

function st() 
    savestate.do_slot(1, "save", function(result, data) end)
    savestate.do_memory("", "save", function(result, data) end)
end

emu.atinterval(st)
emu.atdrawd2d(st)
emu.atupdatescreen(st)
