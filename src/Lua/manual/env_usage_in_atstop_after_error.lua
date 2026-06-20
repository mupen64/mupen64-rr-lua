--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Raises an error in the global scope and calls a function that uses the environment in an atstop callback.
-- Start the script, and ensure that:
-- 1. 'Test passed!' is printed to the console

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

emu.atstop(function()
    print('Test passed!')
end)

error('test')