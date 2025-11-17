--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Provides compatibility shims for old APIs.

-- printx deprecated, forwarded to print
printx = print

-- table.getn deprecated, replaced by # prefix
table.getn = function(t)
    return #t
end

-- emu.debugview deprecated, forwarded to print
emu.debugview = print

-- emu.setgfx deprecated, no-op
emu.setgfx = function(_) end

-- emu.isreadonly deprecated, forwarded to movie.get_readonly
emu.isreadonly = movie.get_readonly

-- emu.getsystemmetrics is not available anymore due to WinAPI coupling concerns.
emu.getsystemmetrics = function() print('emu.getsystemmetrics has been deprecated') end

-- movie.playmovie deprecated, forwarded to movie.play
movie.playmovie = movie.play

-- movie.stopmovie deprecated, forwarded to movie.stop
movie.stopmovie = movie.stop

-- movie.getmoviefilename deprecated, forwarded to movie.get_filename
movie.getmoviefilename = movie.get_filename

-- movie.isreadonly deprecated, forwarded to movie.get_readonly
movie.isreadonly = movie.get_readonly

-- movie.begin_seek_to is not available anymore due to fundamental unshimmable changes in the seek API.
movie.begin_seek_to = function() print('movie.begin_seek_to has been deprecated, use movie.begin_seek instead') end

-- movie.get_seek_info is not available anymore due to fundamental unshimmable changes in the seek API.
movie.get_seek_info = function() print('movie.get_seek_info has been deprecated, use movie.begin_seek instead') end

-- input.map_virtual_key_ex is not available anymore due to WinAPI coupling concerns.
input.map_virtual_key_ex = function() print('input.map_virtual_key_ex has been deprecated') end

-- memory.recompilenow deprecated, forwarded to memory.recompile
memory.recompilenow = memory.recompile

-- memory.recompilenext deprecated, forwarded to memory.recompile
memory.recompilenext = memory.recompile

---Saves a savestate to `filename`.
---@param filename string
---@return nil
---@deprecated This function is not guaranteed to succeed successfully or at any specific point in time. Use `savestate.do_file` instead.
function savestate.savefile(filename)
    savestate.do_file(filename, "save")
end

---Loads a savestate from `filename`.
---@param filename string
---@return nil
---@deprecated This function is not guaranteed to succeed successfully or at any specific point in time. Use `savestate.do_file` instead.
function savestate.loadfile(filename)
    savestate.do_file(filename, "load")
end
