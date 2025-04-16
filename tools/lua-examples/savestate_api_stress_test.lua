function st() 
    savestate.do_slot(1, "save", function(result, data) end)
    savestate.do_memory("", "save", function(result, data) end)
end

emu.atinterval(st)
emu.atdrawd2d(st)
emu.atupdatescreen(st)
