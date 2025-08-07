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
    lust.describe('shims', function()
        lust.describe('table', function()
            lust.it('get_n_works', function()
                lust.expect(table.getn({ 1, 2, 3 })).to.equal(3)
            end)
        end)
    end)

    lust.describe('movie', function()
        lust.describe('play', function()
            lust.it('returns_ok_result_with_non_nil_path', function()
                local result = movie.play("i_dont_exist_but_whatever.m64")
                lust.expect(result).to.equal(Mupen.result.res_ok)
            end)
            lust.it('returns_bad_file_result_with_nil_path', function()
                local result = movie.play(nil)
                lust.expect(result).to.equal(Mupen.result.vcr_bad_file)
            end)
        end)
        lust.describe('stop', function()
            lust.it('returns_anything', function()
                local result = movie.stop()
                lust.expect(result).to.exist()
            end)
        end)
    end)

    lust.describe('actions', function()
        lust.describe('add', function()
            lust.before(function()
                action.remove("Test")
            end)

            lust.it('errors_when_params_are_nil', function()
                local func = function()
                    action.add(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_false_when_params_are_not_table', function()
                local func = function()
                    action.add(4)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_path_missing', function()
                local func = function()
                    action.add({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_params_are_missing_down_callback', function()
                local func = function()
                    action.add({
                        path = "Test > Something"
                    })
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_false_when_path_malformed', function()
                local result = action.add({
                    path = "Test",
                    down_callback = function() end
                })
                lust.expect(result).to.equal(false)
            end)
            lust.it('returns_true_when_params_valid', function()
                local result = action.add({
                    path = "Test > Something",
                    down_callback = function() end
                })
                lust.expect(result).to.equal(true)
            end)
            lust.it('replaces_action_with_existing_path', function()
                local first_called = false
                local second_called = false

                action.add({
                    path = "Test > Something",
                    down_callback = function() first_called = true end,
                })

                action.add({
                    path = "Test > Something",
                    down_callback = function() second_called = true end
                })

                action.invoke("Test > Something")

                lust.expect(first_called).to.equal(false)
                lust.expect(second_called).to.equal(true)
            end)
        end)
        lust.describe('remove', function()
            lust.before(function()
                action.remove("Test")
            end)
            
            lust.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.remove(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_matched_actions_correctly', function()
                local actions = {
                    "Test>1",
                    "Test>2>A",
                    "Test>3",
                    "Test>4>B>C",
                }

                for _, value in pairs(actions) do
                    action.add({
                        path = value,
                        down_callback = function() end,
                    })
                end

                local result = action.remove("Test")

                lust.expect(result).to.equal(actions)
            end)
            lust.it('doesnt_crash_when_action_is_removed_twice', function()
                for i = 1, 2, 1 do
                    action.add({
                        path = "Test>Something",
                        down_callback = function() end
                    })
                    lust.expect(action.remove("Test>Something")).to.equal({ "Test>Something" })
                end
                -- Can't test for crashes in Lua, so this is just a smoke test.
                lust.expect(true).to.be.truthy()
            end)
        end)
    end)
end)
