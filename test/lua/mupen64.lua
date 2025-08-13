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
            lust.after(function()
                action.remove("Test > *")
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
            lust.it('returns_false_when_path_malformed', function()
                local result = action.add({
                    path = "Test",
                })
                lust.expect(result).to.equal(false)
            end)
            lust.it('returns_true_when_params_valid', function()
                local result = action.add({
                    path = "Test > Something",
                })
                lust.expect(result).to.equal(true)
            end)
            lust.it('fails_if_action_already_exists', function()
                action.add({
                    path = "Test > Something",
                })

                local result = action.add({
                    path = "Test > Something",
                })

                lust.expect(result).to.equal(false)
            end)
            lust.it('fails_if_causes_action_to_have_child', function()
                local result = action.add({
                    path = "Test > A",
                })

                lust.expect(result).to.equal(true)

                result = action.add({
                    path = "Test > A > B",
                })

                lust.expect(result).to.equal(false)

                action.remove("Test > *")

                local result = action.add({
                    path = "Test > A > B",
                })

                lust.expect(result).to.equal(true)

                result = action.add({
                    path = "Test > A > B > C > D",
                })

                lust.expect(result).to.equal(false)
            end)
        end)
        lust.describe('remove', function()
            lust.after(function()
                action.remove("Test > *")
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
                    })
                end

                local result = action.remove("Test > *")

                lust.expect(result).to.equal(actions)
            end)
            lust.it('doesnt_crash_when_action_is_removed_twice', function()
                for i = 1, 2, 1 do
                    action.add({
                        path = "Test>Something",
                    })
                    lust.expect(action.remove("Test>Something")).to.equal({ "Test>Something" })
                end
                -- Can't test for crashes in Lua, so this is just a smoke test.
                lust.expect(true).to.be.truthy()
            end)
        end)

        lust.describe('associate_hotkey', function()
            lust.after(function()
                action.remove("Test > *")
            end)

            lust.it('errors_when_path_is_nil', function()
                local func = function()
                    action.associate_hotkey(nil, {})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_path_is_not_string', function()
                local func = function()
                    action.associate_hotkey({}, {})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_hotkey_is_nil', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                    })
                    action.associate_hotkey("Test > Something", nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_hotkey_is_not_table', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                    })
                    action.associate_hotkey("Test > Something", 5)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_overwrite_existing_is_not_boolean', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                    })
                    action.associate_hotkey("Test > Something", 5, 5)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_false_when_action_doesnt_exist', function()
                local result = action.associate_hotkey("Test > Something", {})
                lust.expect(result).to.equal(false)
            end)
            lust.it('returns_false_when_path_isnt_fully_qualified', function()
                action.add({
                    path = "Test > Something",
                })
                local result = action.associate_hotkey("Test > *", {})
                lust.expect(result).to.equal(false)
            end)
            lust.it('works_when_parameters_valid', function()
                action.add({
                    path = "Test > Something",
                })
                local result = action.associate_hotkey("Test > Something", { key = Mupen.VKeycodes.VK_TAB }, true)
                lust.expect(result).to.be.truthy()
            end)
        end)

        lust.describe('batch_work', function()
            lust.it('doesnt_error', function()
                action.begin_batch_work()
                action.end_batch_work()
            end)
        end)

        lust.describe('notify_enabled_changed', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.notify_enabled_changed(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.notify_enabled_changed({})
                end
                lust.expect(func).to.fail()
            end)
            -- A test like "calls_callback_on_affected_actions" is not applicable because we can't know when the callback will be called.
        end)

        lust.describe('notify_active_changed', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.notify_active_changed(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.notify_active_changed({})
                end
                lust.expect(func).to.fail()
            end)
        end)

        lust.describe('notify_display_name_changed', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.notify_display_name_changed(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.notify_display_name_changed({})
                end
                lust.expect(func).to.fail()
            end)
        end)

        lust.describe('get_display_name', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.get_display_name(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.get_display_name({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_correct_name_when_no_action_matched', function()
                local name = action.get_display_name("Test >    Something")
                lust.expect(name).to.equal("Something")
            end)
            lust.it('returns_correct_name_when_no_action_matched_with_separator', function()
                local name = action.get_display_name("Test >    Something ---")
                lust.expect(name).to.equal("Something")
            end)
            lust.it('returns_correct_name_when_action_matched', function()
                action.add({
                    path = "Test > Something",
                })
                local name = action.get_display_name("Test >    Something")
                lust.expect(name).to.equal("Something")
            end)
            lust.it('returns_correct_name_when_action_matched_with_separator', function()
                action.add({
                    path = "Test > Something---",
                })
                local name = action.get_display_name("Test >    Something ---")
                lust.expect(name).to.equal("Something")
            end)
            lust.it('uses_display_name', function()
                action.add({
                    path = "Test > Something",
                    get_display_name = function()
                        return "Hi!"
                    end
                })
                local name = action.get_display_name("Test >    Something")
                lust.expect(name).to.equal("Hi!")
            end)
            lust.it('doesnt_use_display_name_when_ignore_override_true', function()
                action.add({
                    path = "Test > Something",
                    get_display_name = function()
                        return "Hi!"
                    end
                })
                local name = action.get_display_name("Test >    Something", true)
                lust.expect(name).to.equal("Something")
            end)
            lust.it('doesnt_use_display_name_when_ignore_override_true_with_separator', function()
                action.add({
                    path = "Test > Something ---",
                    get_display_name = function()
                        return "Hi!"
                    end
                })
                local name = action.get_display_name("Test >    Something---", true)
                lust.expect(name).to.equal("Something")
            end)
        end)

        lust.describe('get_actions_matching_filter', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.get_actions_matching_filter(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.get_actions_matching_filter({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('matches_even_with_whitespace_and_extra_separators', function()
                action.add({
                    path = "Test>X",
                })

                local result = action.get_actions_matching_filter(" Test  >  X ")
                lust.expect(result).to.equal({ "Test>X" })
            end)
            lust.it('wildcard_requires_additional_segments', function()
                action.add({
                    path = "Test>X",
                })

                local result = action.get_actions_matching_filter("Test > *")
                lust.expect(result).to.equal({ "Test>X" })

                result = action.get_actions_matching_filter("Test > X > *")
                lust.expect(result).to.equal({})
            end)
            lust.it('returns_empty_for_empty_filter', function()
                local result = action.get_actions_matching_filter("")
                lust.expect(result).to.equal({})
            end)
            lust.it('does_not_match_partial_paths_without_wildcard', function()
                action.add({
                    path = "Test>X",
                })

                local result = action.get_actions_matching_filter("Test")
                lust.expect(result).to.equal({})
            end)
            lust.it('returns_correct_actions_wildcard_special_case', function()
                local result = action.get_actions_matching_filter("*")
                -- Flaky: we can't guarantee the number of actions, but we can check that there are roughly enough to be the entire built-in menu.
                lust.expect(#result > 50).to.be.truthy()
            end)
            lust.it('returns_correct_actions', function()
                local actions = {
                    "Test>Something--->A",
                    "Test>B"
                }

                for _, path in pairs(actions) do
                    action.add({
                        path = path,
                    })
                end

                local result

                result = action.get_actions_matching_filter("Test")
                lust.expect(result).to.equal({})

                result = action.get_actions_matching_filter("Test >    *")
                lust.expect(result).to.equal(actions)

                result = action.get_actions_matching_filter("Test  > Something---")
                lust.expect(result).to.equal({})

                result = action.get_actions_matching_filter("Test>Something---> *")
                lust.expect(result).to.equal({
                    "Test>Something--->A"
                })
            end)
        end)

        lust.describe('invoke', function()
            lust.after(function()
                action.remove("Test > *")
            end)

            lust.it('errors_when_path_is_nil', function()
                local func = function()
                    action.invoke(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_path_is_not_string', function()
                local func = function()
                    action.invoke({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('calls_on_press', function()
                local called = false
                action.add({
                    path = "Test > Something",
                    on_press = function()
                        called = true
                    end
                })

                action.invoke("Test > Something")
                lust.expect(called).to.be.truthy()
            end)
            lust.it('calls_on_release', function()
                local called = false
                action.add({
                    path = "Test > Something",
                    on_release = function()
                        called = true
                    end
                })

                action.invoke("Test > Something", true)
                lust.expect(called).to.be.truthy()
            end)
            lust.it('calls_on_release_when_pressing_again_while_pressed', function()
                local down = 0
                local up = 0

                action.add({
                    path = "Test > Something",
                    on_press = function()
                        down = down + 1
                    end,
                    on_release = function()
                        up = up + 1
                    end,
                })

                action.invoke("Test > Something")
                lust.expect(down).to.equal(1)
                lust.expect(up).to.equal(0)

                action.invoke("Test > Something")
                lust.expect(down).to.equal(1)
                lust.expect(up).to.equal(1)
            end)
        end)
    end)

    lust.describe('hotkeys', function()
        lust.describe('prompt', function()
            lust.it('errors_when_caption_nil', function()
                local func = function()
                    hotkeys.prompt(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_caption_not_string', function()
                local func = function()
                    hotkeys.prompt({})
                end
                lust.expect(func).to.fail()
            end)
        end)
    end)
end)
