--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- runs 60 times per second on Super Mario 64
emu.atvi(function()
	local vis = emu.framecount()
	print("VI number " .. vis)
end)
