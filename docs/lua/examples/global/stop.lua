--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- runs constantly
emu.atinterval(function()
	print("Script is running!")
	-- when the S key is pressed
	if input.get().S then
		-- stop the script
		stop()
	end
end)
