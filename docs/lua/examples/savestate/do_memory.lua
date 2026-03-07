local savestate_buffer = ""

savestate.do_memory("", "save", function (result, data)
    assert(result == Mupen.result.res_ok)
    savestate_buffer = data
end)
