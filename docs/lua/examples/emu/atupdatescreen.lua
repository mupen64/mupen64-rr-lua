-- every screen update
emu.atupdatescreen(function()
	-- draw a 30x30 red square at (10, 10)
	wgui.fillrecta(10, 10, 30, 30, "red")
end)
