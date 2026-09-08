--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

local brush = painter.brush({ r = 1, g = 0, b = 0 })

emu.atpaint(function(p)
    p:fill_rect({ x = 10, y = 10, width = 40, height = 40 }, brush)
end)
