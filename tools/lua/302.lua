--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- # Summary
--
-- Mupen crashes if you stop a lua script that opened a file dialog using iohelper.
--
-- # Repro
--
-- 1) Load a lua script that opens a file dialog (e.g. `iohelper.filediag("", 0)`).
-- 2) Stop all lua scripts via Lua Script > Close All
-- 3) Close the file dialog
--
-- # Expected behavior
--
-- Either Mupen should close any file dialogs created by a lua script when stopping it, or closing the dialog should not trigger a crash.
--
-- # Specs
--
-- - CPU: Intel i7 8700k
-- - OS: Windows 10, 22H2

local once = false

emu.atdrawd2d(function()
    if once then
        return
    end
    once = true
    iohelper.filediag("*.*", 0)
end)
