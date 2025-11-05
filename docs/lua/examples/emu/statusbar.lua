emu.statusbar("a cool statusbar message")

emu.atinterval(function()
	if input.get().N then
		emu.statusbar("User just pressed the N key!")
	end
end)
