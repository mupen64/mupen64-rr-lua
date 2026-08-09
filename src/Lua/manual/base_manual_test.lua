--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

--
-- (a repro for the bug here and what to look out for to verify that the manual test passes. can also be a copy-and-pasted issue description)
--

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

-- (your code here)
