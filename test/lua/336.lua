--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

local function evil()
    for i = 1, 100, 1 do
        emu.samplecount()
        emu.inputcount()
        emu.framecount()
        movie.get_seek_completion()
    end
end

emu.atinput(evil)
emu.atvi(evil)
emu.atdrawd2d(evil)
