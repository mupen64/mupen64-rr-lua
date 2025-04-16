print("Press 1 to save to 'out.st'")
print("Press 2 to load from 'out.st'")
print("Press 3 to save to slot 1")
print("Press 4 to load from slot 1")
print("Press 5 to save to memory")
print("Press 6 to load from memory")

local prev_keys = {}
local memory_st = ""

emu.atinterval(function()
    local new_keys = input.diff(input.get(), prev_keys)

    if new_keys["1"] then
        savestate.do_file("out.st", "save", function(result, data)
            print("saved " .. string.len(data) .. " bytes (result " .. result .. ")")
        end)
    elseif new_keys["2"] then
        savestate.do_file("out.st", "load", function(result, data)
            print("loaded " .. string.len(data) .. " bytes (result " .. result .. ")")
        end)
    elseif new_keys["3"] then
        savestate.do_slot(1, "save", function(result, data)
            print("saved " .. string.len(data) .. " bytes (result " .. result .. ")")
        end)
    elseif new_keys["4"] then
        savestate.do_slot(1, "load", function(result, data)
            print("loaded " .. string.len(data) .. " bytes (result " .. result .. ")")
        end)
    elseif new_keys["5"] then
        savestate.do_memory("", "save", function(result, data)
            memory_st = data
            print("saved " .. string.len(data) .. " bytes (result " .. result .. ")")
        end)
    elseif new_keys["6"] then
        savestate.do_memory(memory_st, "load", function(result, data)
            print("loaded " .. string.len(data) .. " bytes (result " .. result .. ")")
        end)
    end

    prev_keys = input.get()
end)
