--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Demonstrates usage of some shimmed APIs.

local t = { 1, 2, 3 }

assert(table.getn(t) == #t)

emu.debugview("hi")

printx("printx() works")

input.map_virtual_key_ex()

emu.getsystemmetrics()

os.execute()

print(string.format("is readonly: %s", emu.isreadonly()))