--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Calls `os.exit`.
-- Start the script, and ensure that:
-- 1. The application exits with no crash

dofile(assert(debug.getinfo(1).source:sub(2):gsub("\\", "/"):match("^(.-)manual/[^/]+$")) .. "test_prelude.lua")

os.exit()
