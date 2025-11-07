--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')


local file = io.open("dummy.txt", "w")
assert(file)
file:write("test")
file:flush()
file:close()

iohelper.filediag("*.*", 0)

file = io.open("dummy.txt", "r")
assert(file,
    "Failed to open dummy.txt for reading. This may indicate that the current working directory was changed unexpectedly by iohelper.filediag.")
file:close()

print("Test passed.")
