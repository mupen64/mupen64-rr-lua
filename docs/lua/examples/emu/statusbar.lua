--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

emu.statusbar("a cool statusbar message")

emu.atinterval(function()
	if input.get().N then
		emu.statusbar("User just pressed the N key!")
	end
end)
