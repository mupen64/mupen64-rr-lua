--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Disables potentially dangerous APIs.

os.execute = function() print("os.execute is not available in an untrusted Lua environment.") end

io.popen = function() print("io.popen is not available in an untrusted Lua environment.") end