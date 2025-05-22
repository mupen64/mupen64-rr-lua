--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Provides compatibility shims for old APIs.

-- table.getn deprecated, replaced by # prefix
table.getn = function(t)
    return #t
end

-- emu.debugview deprecated, forwarded to print
emu.debugview = print

-- emu.setgfx deprecated, no-op
emu.setgfx = function (_) end

-- movie.playmovie deprecated, forwarded to movie.play
movie.playmovie = movie.play

-- movie.stopmovie deprecated, forwarded to movie.stop
movie.stopmovie = movie.stop

-- movie.getmoviefilename deprecated, forwarded to movie.get_filename
movie.getmoviefilename = movie.get_filename

-- movie.isreadonly deprecated, forwarded to movie.get_readonly
movie.isreadonly = movie.get_readonly

-- emu.isreadonly deprecated, forwarded to movie.get_readonly
emu.isreadonly = movie.get_readonly

-- printx deprecated, forwarded to print
printx = print

-- input.map_virtual_key_ex is not available anymore due to WinAPI coupling concerns.
input.map_virtual_key_ex = function() print('input.map_virtual_key_ex has been deprecated') end

-- emu.getsystemmetrics is not available anymore due to WinAPI coupling concerns.
emu.getsystemmetrics = function() print('emu.getsystemmetrics has been deprecated') end

-- movie.begin_seek_to is not available anymore due to fundamental unshimmable changes in the seek API.
movie.begin_seek_to = function() print('movie.begin_seek_to has been deprecated, use movie.begin_seek instead') end

-- movie.get_seek_info is not available anymore due to fundamental unshimmable changes in the seek API.
movie.get_seek_info = function() print('movie.get_seek_info has been deprecated, use movie.begin_seek instead') end

-- movie.get_seek_info is not available anymore due to security risks.
os.execute = function() print('os.execute is disabled') end