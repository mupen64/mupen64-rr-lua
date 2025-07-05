--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Tries to call dangerous APIs. These calls shouldn't work in an untrusted Lua environment.

os.execute("start calc.exe")

io.popen("start calc.exe")

os.remove("a.txt")

os.rename("a.txt", "b.txt")