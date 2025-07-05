--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Tries to call APIs protected by the sandbox. Run this script in untrusted mode and verify that all assertions pass.

local tests = {}
tests[#tests + 1] = function()
    local suc, exitcode, code = os.execute("start calc.exe")

    assert(suc == false)
    assert(exitcode == nil)
    assert(code == nil)
end
tests[#tests + 1] = function()
    local file, err = io.popen("start calc.exe")

    assert(file == nil)
    assert(err == nil)
end
tests[#tests + 1] = function()
    local suc, err = os.remove("a.txt")

    assert(suc == false)
    assert(err == nil)
end
tests[#tests + 1] = function()
    local suc, err = os.rename("a.txt", "b.txt")

    assert(suc == false)
    assert(err == nil)
end

for key, value in pairs(tests) do
    value()
end
