--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

---
--- Describes the testing suite for the Mupen64 Lua API.
---

dofile(debug.getinfo(1).source:sub(2):gsub("[^\\]+$", "") .. 'prelude.lua')

lust.describe('mupen64', function()
    lust.describe('movie', function()
        lust.it('play_returns_res_ok', function()
            lust.expect(movie.play("i_dont_exist_but_whatever.m64")).to.equal(Mupen.result.res_ok)
        end)

        lust.it('play_with_nil_returns_vcr_bad_file', function()
            lust.expect(movie.play(nil)).to.equal(Mupen.result.vcr_bad_file)
        end)
    end)
end)
