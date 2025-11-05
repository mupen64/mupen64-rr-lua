local brush = d2d.create_brush(1, 0, 0, 1)

emu.atdrawd2d(function()
	d2d.fill_rectangle(10, 10, 40, 40, brush)
end)

emu.atstop(function()
	d2d.free_brush(brush)
end)
