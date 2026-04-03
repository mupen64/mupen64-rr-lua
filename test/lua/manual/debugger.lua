--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--


dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local bp

bp = debugger.add_breakpoint(2147483756, function(state)
    print('breakpoint hit', state)
    print('removing...')
    debugger.remove_breakpoint(bp)
end)
