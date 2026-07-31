--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

local savestate_buffer = ""

savestate.do_memory("", "save", function (result, data)
    assert(result == Mupen.result.res_ok)
    savestate_buffer = data
end)
