--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

emu.atpaint(function(p)
    p:begin_path()
    p:rect({ x = 10, y = 10, w = 40, h = 40 })
    p:fill({ r = 1, g = 0, b = 0 })
end)
