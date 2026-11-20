--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Empty script used for benchmarking Lua overhead.

dofile(assert(debug.getinfo(1).source:sub(2):gsub("\\", "/"):match("^(.-)manual/[^/]+$")) .. "test_prelude.lua")
