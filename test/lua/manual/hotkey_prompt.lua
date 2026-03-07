--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Calls `hotkey.prompt`.
-- Start the script, and ensure that:
-- 1. A prompt appears asking to "Choose a hotkey"
-- 2. Choosing a hotkey prints it
-- 3. Cancelling the prompt prints nil

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local hotkey = hotkey.prompt("Choose a hotkey")

print(hotkey)