--
-- Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

---
--- Describes the automated testing suite for the Mupen64 Lua API.
--- Assumes an x86 Windows environment with no Lua trust.
---

dofile(debug.getinfo(1).source:sub(2):gsub("[^\\]+$", "") .. 'test_prelude.lua')

local testlib_path = path_root .. "build\\test\\Lua.Testlib\\"
local testlib_dll_path = testlib_path .. "luatestlib.dll"
package.cpath = testlib_path .. "?.dll;" .. package.cpath

lust.describe('mupen64', function()
    lust.describe('shims', function()
        lust.describe('global', function()
            lust.it('printx_forwarded_to_print', function()
                lust.expect(printx).to.equal(print)
            end)
        end)
        lust.describe('table', function()
            lust.it('get_n_works', function()
                lust.expect(table.getn({ 1, 2, 3 })).to.equal(3)
            end)
        end)
        lust.describe('emu', function()
            lust.it('debug_view_forwarded_to_print', function()
                lust.expect(emu.debugview).to.equal(print)
            end)
            lust.it('setgfx_exists_and_is_noop', function()
                local func = function()
                    emu.setgfx("anything")
                end
                lust.expect(func).to_not.fail()
            end)
            lust.it('isreadonly_forwarded_to_movie_get_readonly', function()
                lust.expect(emu.isreadonly).to.equal(movie.get_readonly)
            end)
            lust.it('getsystemmetrics_exists_and_prints_deprecation', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                emu.getsystemmetrics()

                print = __prev_print
                lust.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
            lust.describe('getversion', function()
                lust.it('full_version_equals_meta_version_with_prefix', function()
                    local version = emu.getversion(0)
                    lust.expect(version).to.equal("Mupen 64 " .. Mupen._VERSION)
                end)
                lust.it('short_version_equals_meta_version', function()
                    local version = emu.getversion(1)
                    lust.expect(version).to.equal(Mupen._VERSION)
                end)
            end)
        end)
        lust.describe('movie', function()
            lust.it('playmovie_forwarded_to_play', function()
                lust.expect(movie.playmovie).to.equal(movie.play)
            end)
            lust.it('stopmovie_forwarded_to_stop', function()
                lust.expect(movie.stopmovie).to.equal(movie.stop)
            end)
            lust.it('getmoviefilename_forwarded_to_get_filename', function()
                lust.expect(movie.getmoviefilename).to.equal(movie.get_filename)
            end)
            lust.it('isreadonly_forwarded_to_get_readonly', function()
                lust.expect(movie.isreadonly).to.equal(movie.get_readonly)
            end)
            lust.it('begin_seek_to_exists_and_prints_deprecation', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                movie.begin_seek_to()

                print = __prev_print
                lust.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
            lust.it('get_seek_info_exists_and_prints_deprecation', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                movie.get_seek_info()

                print = __prev_print
                lust.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
        end)
        lust.describe('input', function()
            lust.it('map_virtual_key_ex_exists_and_prints_deprecation', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                input.map_virtual_key_ex()

                print = __prev_print
                lust.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
        end)
        lust.describe('memory', function()
            lust.it('recompilenow_forwarded_to_recompile', function()
                lust.expect(memory.recompilenow).to.equal(memory.recompile)
            end)
            lust.it('recompilenext_forwarded_to_recompile', function()
                lust.expect(memory.recompilenext).to.equal(memory.recompile)
            end)
        end)
    end)

    lust.describe('wgui', function()
        local TEST_ROOT = "../../test/lua/"
        local VALID_IMAGE = TEST_ROOT .. "image.png"
        local NONEXISTENT_IMAGE = TEST_ROOT .. "nonexistent.png"
        local OUTPUT_IMAGE_PNG = TEST_ROOT .. "output.png"

        lust.describe('loadimage', function()
            lust.it('loads_valid_image', function()
                local idx = wgui.loadimage(VALID_IMAGE)
                lust.expect(idx).to.be.a('number')
            end)
            lust.it('errors_loading_nonexistent_image', function()
                local func = function()
                    wgui.loadimage(NONEXISTENT_IMAGE)
                end
                lust.expect(func).to.fail()
            end)
        end)

        lust.describe('saveimage', function()
            lust.it('saves_loaded_image', function()
                local idx = wgui.loadimage(VALID_IMAGE)
                local result = wgui.saveimage(idx, OUTPUT_IMAGE_PNG)
                lust.expect(result).to.equal(true)
            end)
            lust.it('errors_saving_non_loaded_image', function()
                local func = function()
                    wgui.saveimage(9999, OUTPUT_IMAGE_PNG)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_saving_unsupported_format', function()
                local idx = wgui.loadimage(VALID_IMAGE)
                local result = wgui.saveimage(idx, TEST_ROOT .. "output_image.abcdefxyz")
                lust.expect(result).to.equal(false)
            end)
        end)
    end)

    lust.describe('d2d', function()
        lust.describe('draw_to_image', function()
            lust.it('clamps_negative_sizes', function()
                local img = d2d.draw_to_image(-10, -10, function() end)
                local info = d2d.get_image_info(img)
                lust.expect(info.width).to.equal(1)
                lust.expect(info.height).to.equal(1)
            end)
        end)
        lust.describe('draw_text', function()
            lust.it('doesnt_crash_with_negative_sizes', function()
                local brush = d2d.create_brush(1, 0, 0, 1)
                d2d.draw_text(0, 0, -10, -10, "Test", "Arial", 12, 800, 0, 0, 0, 0, brush)
                d2d.free_brush(brush)
                lust.expect(true).to.be.truthy()
            end)
        end)
    end)

    lust.describe('trust', function()
        local function print_test_wrapper(func)
            __prev_print = print
            local printed_str
            print = function(str) printed_str = str end

            func()

            print = __prev_print
            lust.expect(printed_str:find("dangerous") ~= nil).to.equal(true)
        end
        local function print_suppression_wrapper_begin()
            __prev_print = print
            print = function() end
        end
        local function print_suppression_wrapper_end()
            print = __prev_print
        end

        lust.describe('os.execute', function()
            lust.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local suc, exitcode, code = os.execute("start calc.exe")
                print_suppression_wrapper_end()

                lust.expect(suc).to.equal(false)
                lust.expect(exitcode).to.equal(nil)
                lust.expect(code).to.equal(nil)
            end)
            lust.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    os.execute("start calc.exe")
                end)
            end)
        end)
        lust.describe('io.popen', function()
            lust.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local file, err = io.popen("start calc.exe")
                print_suppression_wrapper_end()

                lust.expect(file).to.equal(nil)
                lust.expect(err).to.equal(nil)
            end)
            lust.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local file, err = io.popen("start calc.exe")
                end)
            end)
        end)
        lust.describe('os.remove', function()
            lust.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local suc, err = os.remove("a.txt")
                print_suppression_wrapper_end()

                lust.expect(suc).to.equal(false)
                lust.expect(err).to.equal(nil)
            end)
            lust.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local suc, err = os.remove("a.txt")
                end)
            end)
        end)
        lust.describe('os.rename', function()
            lust.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local suc, err = os.rename("a.txt", "b.txt")
                print_suppression_wrapper_end()

                lust.expect(suc).to.equal(false)
                lust.expect(err).to.equal(nil)
            end)
            lust.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local suc, err = os.rename("a.txt", "b.txt")
                end)
            end)
        end)
        lust.describe('package.loadlib', function()
            lust.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local lib = package.loadlib(testlib_dll_path, "luaopen_testlib")
                print_suppression_wrapper_end()

                lust.expect(lib).to.equal(nil)
            end)
            lust.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local lib = package.loadlib(testlib_dll_path, "luaopen_testlib")
                end)
            end)
        end)
        -- lust.describe('require', function()
        --     lust.it('returns_correct_values_in_untrusted_environment', function()
        --         print_suppression_wrapper_begin()
        --         local lib = require("socket.core")
        --         print_suppression_wrapper_end()

        --         lust.expect(lib).to.equal(nil)
        --     end)
        --     lust.it('prints_warning_message_in_untrusted_environment', function()
        --         print_test_wrapper(function()
        --             local lib = require("socket.core")
        --         end)
        --     end)
        -- end)
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

    lust.describe('input', function()
        lust.describe('get_key_name_text', function()
            -- NOTE: This test only works on an en-us locale.
            lust.it('returns_correct_value', function()
                lust.expect(input.get_key_name_text(Mupen.VKeycodes.VK_1)).to.equal("1")
                lust.expect(input.get_key_name_text(Mupen.VKeycodes.VK_RETURN)).to.equal("Enter")
                lust.expect(input.get_key_name_text(Mupen.VKeycodes.VK_SPACE)).to.equal("Space")
                lust.expect(input.get_key_name_text(Mupen.VKeycodes.VK_DOWN)).to.equal("Down")
                lust.expect(input.get_key_name_text(string.byte('W'))).to.equal("W")
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

        lust.describe('get_enabled', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_enabled(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_enabled({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_false_when_action_doesnt_exist', function()
                local result = action.get_enabled("Test > Something")
                lust.expect(result).to.equal(false)
            end)
            lust.it('returns_enabled_state', function()
                action.add({
                    path = "Test > A",
                    get_enabled = function()
                        return true
                    end
                })
                action.add({
                    path = "Test > B",
                    get_enabled = function()
                        return false
                    end
                })
                lust.expect(action.get_enabled("Test > A")).to.equal(true)
                lust.expect(action.get_enabled("Test > B")).to.equal(false)
            end)
        end)

        lust.describe('get_active', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_active(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_active({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_false_when_action_doesnt_exist', function()
                local result = action.get_active("Test > Something")
                lust.expect(result).to.equal(false)
            end)
            lust.it('returns_active_state', function()
                action.add({
                    path = "Test > A",
                    get_active = function()
                        return true
                    end
                })
                action.add({
                    path = "Test > B",
                    get_active = function()
                        return false
                    end
                })
                lust.expect(action.get_active("Test > A")).to.equal(true)
                lust.expect(action.get_active("Test > B")).to.equal(false)
            end)
        end)

        lust.describe('get_activatability', function()
            lust.after(function()
                action.remove("Test > *")
            end)
            lust.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_activatability(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_activatability({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_false_when_action_doesnt_exist', function()
                local result = action.get_activatability("Test > Something")
                lust.expect(result).to.equal(false)
            end)
            lust.it('returns_true_when_get_active_callback_present', function()
                action.add({
                    path = "Test > Something",
                    get_active = function()
                        return true
                    end
                })
                local result = action.get_activatability("Test > Something")
                lust.expect(result).to.equal(true)
            end)
            lust.it('returns_false_when_get_active_callback_absent', function()
                action.add({
                    path = "Test > Something",
                })
                local result = action.get_activatability("Test > Something")
                lust.expect(result).to.equal(false)
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
            lust.it('doesnt_call_release_when_pressing_again_while_pressed_if_release_on_repress_is_false', function()
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

                action.invoke("Test > Something", false, false)
                lust.expect(down).to.equal(2)
                lust.expect(up).to.equal(0)
            end)
        end)

        lust.describe('lock_hotkeys', function()
            lust.it('errors_when_lock_nil', function()
                local func = function()
                    action.lock_hotkeys(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_when_lock_not_bool', function()
                local func = function()
                    action.lock_hotkeys({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('succeeds_when_lock_bool', function()
                local func = function()
                    action.lock_hotkeys(false)
                end
                lust.expect(func).to_not.fail()
            end)
        end)

        lust.describe('get_hotkeys_locked', function()
            lust.it('returns_locked_state', function()
                action.lock_hotkeys(true)
                lust.expect(action.get_hotkeys_locked()).to.equal(true)
                action.lock_hotkeys(false)
                lust.expect(action.get_hotkeys_locked()).to.equal(false)
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

    lust.describe('clipboard', function()
        lust.describe('get', function()
            lust.it('errors_if_type_nil', function()
                clipboard.clear()
                local func = function()
                    clipboard.get(nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_if_type_not_string', function()
                clipboard.clear()
                local func = function()
                    clipboard.get({})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_if_type_doesnt_exist', function()
                clipboard.clear()
                local func = function()
                    clipboard.get("blah blah blah")
                end
                lust.expect(func).to.fail()
            end)
            lust.it('returns_nil_if_clipboard_empty', function()
                clipboard.clear()
                lust.expect(clipboard.get("text")).to.equal(nil)
            end)
            lust.it('returns_clipboard_value', function()
                clipboard.set("text", "test")
                lust.expect(clipboard.get("text")).to.equal("test")
            end)
        end)
        lust.describe('get_content_type', function()
            lust.it('returns_nil_if_empty', function()
                clipboard.clear()
                lust.expect(clipboard.get_content_type()).to.equal(nil)
            end)
            lust.it('returns_content_type', function()
                clipboard.set("text", "test")
                lust.expect(clipboard.get_content_type()).to.equal("text")
            end)
        end)
        lust.describe('set', function()
            lust.it('errors_if_type_nil', function()
                clipboard.clear()
                local func = function()
                    clipboard.set(nil, "test")
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_if_type_not_string', function()
                clipboard.clear()
                local func = function()
                    clipboard.set({}, "test")
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_if_type_doesnt_exist', function()
                clipboard.clear()
                local func = function()
                    clipboard.set("blah blah blah", "test")
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_if_value_nil', function()
                clipboard.clear()
                local func = function()
                    clipboard.set("text", nil)
                end
                lust.expect(func).to.fail()
            end)
            lust.it('errors_if_value_not_string', function()
                clipboard.clear()
                local func = function()
                    clipboard.set("text", {})
                end
                lust.expect(func).to.fail()
            end)
            lust.it('get_returns_set_value_after', function()
                clipboard.set("text", "test")
                lust.expect(clipboard.get("text")).to.equal("test")
            end)
            lust.it('overwrites_existing_text', function()
                clipboard.set("text", "test")
                lust.expect(clipboard.get("text")).to.equal("test")

                clipboard.set("text", "test2")
                lust.expect(clipboard.get("text")).to.equal("test2")
            end)
        end)
        lust.describe('clear', function()
            lust.it('get_returns_nil_after', function()
                clipboard.clear()
                lust.expect(clipboard.get("text")).to.equal(nil)
            end)
        end)
    end)
end)
