--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Disables potentially dangerous APIs.

local function warn_trust(func_name)
    print(string.format("Blocked call to '%s': this function is dangerous and not allowed in untrusted Lua environments. To enable it, turn on 'Trusted Mode' from the 'Start' dropdown.", func_name))
end

os.execute = function()
    warn_trust("os.execute")
    return false, nil, nil
end

io.popen = function()
    warn_trust("io.popen")
    return nil, nil
end

os.remove = function()
    warn_trust("os.remove")
    return false, nil
end

os.rename = function()
    warn_trust("os.rename")
    return false, nil
end

package.loadlib = function()
    warn_trust("package.loadlib")
    return nil
end

-- TODO: Proper sandboxing of `require`!