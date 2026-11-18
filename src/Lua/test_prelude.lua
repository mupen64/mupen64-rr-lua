--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Normalize separators so path handling works the same on every platform.
local source_path = debug.getinfo(1).source:sub(2):gsub("\\", "/")

-- Repository root, with a trailing separator.
path_root = assert(source_path:match("^(.-)src/Lua/[^/]+$"))
-- Directory containing this file, with a trailing separator.
cpath = assert(source_path:match("^(.-)[^/]*$"))
lib_path = path_root .. "vendor/lua-modules/"

---@module "retest"
retest = dofile(lib_path .. 'retest.lua')
