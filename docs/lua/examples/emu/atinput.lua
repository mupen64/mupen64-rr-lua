local counter = 0;
emu.atinput(function()
	if counter % 2 == 0 then
		joypad.set(1, {A = true})
	end
	counter = counter + 1
end)
