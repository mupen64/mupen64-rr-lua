--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

local function trim_to_nth_last_component(path, n)
    local components = {}
    for component in path:gmatch("[^\\]+") do
        table.insert(components, component)
    end

    local trimmed = table.concat(components, "\\", 1, #components - n)
    if path:sub(2, 2) == ":" then
        trimmed = trimmed .. "\\"
    end
    return trimmed
end

local path_root = trim_to_nth_last_component(debug.getinfo(1).source:sub(2), 3)

lib_path = path_root .. "\\lib\\"

---@module "lust"
lust = dofile(lib_path .. 'lust.lua').nocolor()