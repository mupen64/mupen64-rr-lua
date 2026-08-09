--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- every screen update
emu.atupdatescreen(function()
	-- draw a 30x30 red square at (10, 10)
	wgui.fillrecta(10, 10, 30, 30, "red")
end)
