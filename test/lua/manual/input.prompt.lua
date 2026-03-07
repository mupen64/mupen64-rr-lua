--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Calls `input.prompt`.
-- Start the script, and ensure that:
-- 1. The dialog's title is "works" and the pre-filled editbox value is "works2".
-- 2. When you press OK, the assertion passes.
-- 3. The dialog's title is "input:" and the pre-filled editbox value is "".
-- 4. When you cancel the dialog, the assertion passes.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

assert(input.prompt("works", "works2") == "works2")
assert(input.prompt(nil, nil) == nil)
