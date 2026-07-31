--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

local counter = 0;
emu.atinput(function()
	if counter % 2 == 0 then
		joypad.set(1, {A = true})
	end
	counter = counter + 1
end)
