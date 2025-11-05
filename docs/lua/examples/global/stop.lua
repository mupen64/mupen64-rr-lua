-- runs constantly
emu.atinterval(function()
	print("Script is running!")
	-- when the S key is pressed
	if input.get().S then
		-- stop the script
		stop()
	end
end)
