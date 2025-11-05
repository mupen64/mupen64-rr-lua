-- runs 60 times per second on Super Mario 64
emu.atvi(function()
	local vis = emu.framecount()
	print("VI number " .. vis)
end)
