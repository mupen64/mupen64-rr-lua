--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
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

retest.attach_coverage(
    {
        { "emu",       emu },
        { "memory",    memory },
        { "wgui",      wgui },
        { "d2d",       d2d },
        { "input",     input },
        { "joypad",    joypad },
        { "movie",     movie },
        { "savestate", savestate },
        { "iohelper",  iohelper },
        { "avi",       avi },
        { "hotkey",    hotkey },
        { "action",    action },
        { "clipboard", clipboard },
    }
)

retest.describe('mupen64', function()
    retest.describe('printx', function()
        retest.it('forwarded_to_print', function()
            retest.expect(printx).to.equal(print)
        end)
    end)
    retest.describe('unpack', function()
        retest.it('works', function()
            local t = { 1, 2 }

            local function sum(a, b)
                return a + b
            end

            local result = sum(unpack(t))
            retest.expect(result).to.equal(3)
        end)
    end)
    retest.describe('math', function()
        retest.describe('atan2', function()
            retest.it('works', function()
                local y, x = 1, 1
                local result = math.atan2(y, x)
                retest.expect(result).to.equal(math.pi / 4)
            end)
        end)
        retest.describe('pow', function()
            retest.it('works', function()
                retest.expect(math.pow(5, 6)).to.equal(5 ^ 6)
            end)
        end)
    end)
    retest.describe('table', function()
        retest.describe('getn', function()
            retest.it('works', function()
                retest.expect(table.getn({ 1, 2, 3 })).to.equal(3)
            end)
        end)
    end)
    retest.describe('emu', function()
        retest.describe('debugview', function()
            retest.it('forwarded_to_print', function()
                retest.expect(emu.debugview).to.equal(print)
            end)
        end)

        retest.describe('setgfx', function()
            retest.it('exists_and_is_noop', function()
                local func = function()
                    emu.setgfx("anything")
                end
                retest.expect(func).to_not.fail()
            end)
        end)

        retest.describe('isreadonly', function()
            retest.it('forwarded_to_movie_get_readonly', function()
                retest.expect(emu.isreadonly).to.equal(movie.get_readonly)
            end)
        end)

        retest.describe('getsystemmetrics', function()
            retest.it('exists_and_prints_deprecation_warning', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                emu.getsystemmetrics()

                print = __prev_print
                retest.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
        end)

        retest.describe('getversion', function()
            retest.it('full_version_equals_meta_version_with_prefix', function()
                local version = emu.getversion(0)
                retest.expect(version).to.equal("Mupen 64 " .. Mupen._VERSION)
            end)
            retest.it('short_version_equals_meta_version', function()
                local version = emu.getversion(1)
                retest.expect(version).to.equal(Mupen._VERSION)
            end)
        end)
    end)
    retest.describe('movie', function()
        retest.describe('playmovie', function()
            retest.it('forwarded_to_play', function()
                retest.expect(movie.playmovie).to.equal(movie.play)
            end)
        end)

        retest.describe('stopmovie', function()
            retest.it('forwarded_to_stop', function()
                retest.expect(movie.stopmovie).to.equal(movie.stop)
            end)
        end)

        retest.describe('getmoviefilename', function()
            retest.it('forwarded_to_get_filename', function()
                retest.expect(movie.getmoviefilename).to.equal(movie.get_filename)
            end)
        end)

        retest.describe('isreadonly', function()
            retest.it('forwarded_to_get_readonly', function()
                retest.expect(movie.isreadonly).to.equal(movie.get_readonly)
            end)
        end)

        retest.describe('begin_seek_to', function()
            retest.it('exists_and_prints_deprecation_warning', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                movie.begin_seek_to()

                print = __prev_print
                retest.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
        end)

        retest.describe('get_seek_info', function()
            retest.it('exists_and_prints_deprecation_warning', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                movie.get_seek_info()

                print = __prev_print
                retest.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
        end)
    end)

    retest.describe('savestate', function()
        retest.describe('savefile', function()
            retest.it('calls_dofile', function()
                local FILENAME = "test.st"

                __prev_savestate_do_file = savestate.do_file
                savestate.do_file = function(filename, mode, callback)
                    retest.expect(filename).to.equal(FILENAME)
                    retest.expect(mode).to.equal("save")
                    retest.expect(callback).to.be.a('function')
                end

                savestate.savefile(FILENAME)

                savestate.do_file = __prev_savestate_do_file
            end)
        end)

        retest.describe('loadfile', function()
            retest.it('calls_dofile', function()
                local FILENAME = "test.st"

                __prev_savestate_do_file = savestate.do_file
                savestate.do_file = function(filename, mode, callback)
                    retest.expect(filename).to.equal(FILENAME)
                    retest.expect(mode).to.equal("load")
                    retest.expect(callback).to.be.a('function')
                end

                savestate.loadfile(FILENAME)

                savestate.do_file = __prev_savestate_do_file
            end)
        end)
    end)

    retest.describe('input', function()
        retest.describe('map_virtual_key_ex', function()
            retest.it('exists_and_prints_deprecation_warning', function()
                __prev_print = print
                local printed_str
                print = function(str) printed_str = str end

                input.map_virtual_key_ex()

                print = __prev_print
                retest.expect(printed_str:find("deprecated") ~= nil).to.equal(true)
            end)
        end)
    end)

    retest.describe('memory', function()
        retest.describe('recompilenow', function()
            retest.it('forwarded_to_recompile', function()
                retest.expect(memory.recompilenow).to.equal(memory.recompile)
            end)
        end)
        retest.describe('recompilenext', function()
            retest.it('forwarded_to_recompile', function()
                retest.expect(memory.recompilenext).to.equal(memory.recompile)
            end)
        end)
    end)

    retest.describe('wgui', function()
        local TEST_ROOT = "../../test/lua/"
        local VALID_IMAGE = TEST_ROOT .. "image.png"
        local NONEXISTENT_IMAGE = TEST_ROOT .. "nonexistent.png"
        local OUTPUT_IMAGE_PNG = TEST_ROOT .. "output.png"

        retest.describe('loadimage', function()
            retest.it('loads_valid_image', function()
                local idx = wgui.loadimage(VALID_IMAGE)
                retest.expect(idx).to.be.a('number')
            end)
            retest.it('errors_loading_nonexistent_image', function()
                local func = function()
                    wgui.loadimage(NONEXISTENT_IMAGE)
                end
                retest.expect(func).to.fail()
            end)
        end)

        retest.describe('saveimage', function()
            retest.it('saves_loaded_image', function()
                local idx = wgui.loadimage(VALID_IMAGE)
                local result = wgui.saveimage(idx, OUTPUT_IMAGE_PNG)
                retest.expect(result).to.equal(true)
            end)
            retest.it('errors_saving_non_loaded_image', function()
                local func = function()
                    wgui.saveimage(9999, OUTPUT_IMAGE_PNG)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_saving_unsupported_format', function()
                local idx = wgui.loadimage(VALID_IMAGE)
                local result = wgui.saveimage(idx, TEST_ROOT .. "output_image.abcdefxyz")
                retest.expect(result).to.equal(false)
            end)
        end)
    end)

    retest.describe('d2d', function()
        retest.describe('draw_to_image', function()
            retest.it('clamps_negative_sizes', function()
                local img = d2d.draw_to_image(-10, -10, function() end)
                local info = d2d.get_image_info(img)
                retest.expect(info.width).to.equal(1)
                retest.expect(info.height).to.equal(1)
            end)
        end)
        retest.describe('draw_text', function()
            retest.it('doesnt_crash_with_negative_sizes', function()
                local brush = d2d.create_brush(1, 0, 0, 1)
                d2d.draw_text(0, 0, -10, -10, "Test", "Arial", 12, 800, 0, 0, 0, 0, brush)
                d2d.free_brush(brush)
                retest.expect(true).to.be.truthy()
            end)
        end)
    end)

    retest.describe('trust', function()
        local function print_test_wrapper(func)
            __prev_print = print
            local printed_str
            print = function(str) printed_str = str end

            func()

            print = __prev_print
            retest.expect(printed_str:find("dangerous") ~= nil).to.equal(true)
        end
        local function print_suppression_wrapper_begin()
            __prev_print = print
            print = function() end
        end
        local function print_suppression_wrapper_end()
            print = __prev_print
        end

        retest.describe('os.execute', function()
            retest.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local suc, exitcode, code = os.execute("start calc.exe")
                print_suppression_wrapper_end()

                retest.expect(suc).to.equal(false)
                retest.expect(exitcode).to.equal(nil)
                retest.expect(code).to.equal(nil)
            end)
            retest.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    os.execute("start calc.exe")
                end)
            end)
        end)
        retest.describe('io.popen', function()
            retest.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local file, err = io.popen("start calc.exe")
                print_suppression_wrapper_end()

                retest.expect(file).to.equal(nil)
                retest.expect(err).to.equal(nil)
            end)
            retest.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local file, err = io.popen("start calc.exe")
                end)
            end)
        end)
        retest.describe('os.remove', function()
            retest.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local suc, err = os.remove("a.txt")
                print_suppression_wrapper_end()

                retest.expect(suc).to.equal(false)
                retest.expect(err).to.equal(nil)
            end)
            retest.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local suc, err = os.remove("a.txt")
                end)
            end)
        end)
        retest.describe('os.rename', function()
            retest.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local suc, err = os.rename("a.txt", "b.txt")
                print_suppression_wrapper_end()

                retest.expect(suc).to.equal(false)
                retest.expect(err).to.equal(nil)
            end)
            retest.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local suc, err = os.rename("a.txt", "b.txt")
                end)
            end)
        end)
        retest.describe('package.loadlib', function()
            retest.it('returns_correct_values_in_untrusted_environment', function()
                print_suppression_wrapper_begin()
                local lib = package.loadlib(testlib_dll_path, "luaopen_testlib")
                print_suppression_wrapper_end()

                retest.expect(lib).to.equal(nil)
            end)
            retest.it('prints_warning_message_in_untrusted_environment', function()
                print_test_wrapper(function()
                    local lib = package.loadlib(testlib_dll_path, "luaopen_testlib")
                end)
            end)
        end)
        -- retest.describe('require', function()
        --     retest.it('returns_correct_values_in_untrusted_environment', function()
        --         print_suppression_wrapper_begin()
        --         local lib = require("socket.core")
        --         print_suppression_wrapper_end()

        --         retest.expect(lib).to.equal(nil)
        --     end)
        --     retest.it('prints_warning_message_in_untrusted_environment', function()
        --         print_test_wrapper(function()
        --             local lib = require("socket.core")
        --         end)
        --     end)
        -- end)
    end)

    retest.describe('movie', function()
        retest.describe('play', function()
            retest.it('returns_ok_result_with_non_nil_path', function()
                local result = movie.play("i_dont_exist_but_whatever.m64")
                retest.expect(result).to.equal(Mupen.result.res_ok)
            end)
            retest.it('returns_bad_file_result_with_nil_path', function()
                local result = movie.play(nil)
                retest.expect(result).to.equal(Mupen.result.vcr_bad_file)
            end)
        end)
        retest.describe('stop', function()
            retest.it('returns_anything', function()
                local result = movie.stop()
                retest.expect(result).to.exist()
            end)
        end)
    end)

    retest.describe('input', function()
        retest.describe('get_key_name_text', function()
            -- NOTE: This test only works on an en-us locale.
            retest.it('returns_correct_value', function()
                retest.expect(input.get_key_name_text(Mupen.VKeycodes.VK_1)).to.equal("1")
                retest.expect(input.get_key_name_text(Mupen.VKeycodes.VK_RETURN)).to.equal("Enter")
                retest.expect(input.get_key_name_text(Mupen.VKeycodes.VK_SPACE)).to.equal("Space")
                retest.expect(input.get_key_name_text(Mupen.VKeycodes.VK_DOWN)).to.equal("Down")
                retest.expect(input.get_key_name_text(string.byte('W'))).to.equal("W")
            end)
        end)
    end)

    retest.describe('actions', function()
        retest.describe('add', function()
            retest.after(function()
                action.remove("Test > *")
            end)

            retest.it('errors_when_params_are_nil', function()
                local func = function()
                    action.add(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_false_when_params_are_not_table', function()
                local func = function()
                    action.add(4)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_missing', function()
                local func = function()
                    action.add({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_false_when_path_malformed', function()
                local result = action.add({
                    path = "Test",
                })
                retest.expect(result).to.equal(false)
            end)
            retest.it('returns_true_when_params_valid', function()
                local result = action.add({
                    path = "Test > Something",
                })
                retest.expect(result).to.equal(true)
            end)
            retest.it('errors_when_params_malformed', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                        params = "not_a_table",
                    })
                end
                retest.expect(func).to.fail()
            end)
            retest.it('fails_if_action_already_exists', function()
                action.add({
                    path = "Test > Something",
                })

                local result = action.add({
                    path = "Test > Something",
                })

                retest.expect(result).to.equal(false)
            end)
            retest.it('fails_if_causes_action_to_have_child', function()
                local result = action.add({
                    path = "Test > A",
                })

                retest.expect(result).to.equal(true)

                result = action.add({
                    path = "Test > A > B",
                })

                retest.expect(result).to.equal(false)

                action.remove("Test > *")

                local result = action.add({
                    path = "Test > A > B",
                })

                retest.expect(result).to.equal(true)

                result = action.add({
                    path = "Test > A > B > C > D",
                })

                retest.expect(result).to.equal(false)
            end)
        end)
        retest.describe('remove', function()
            retest.after(function()
                action.remove("Test > *")
            end)

            retest.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.remove(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_matched_actions_correctly', function()
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

                retest.expect(result).to.equal(actions)
            end)
            retest.it('doesnt_crash_when_action_is_removed_twice', function()
                for i = 1, 2, 1 do
                    action.add({
                        path = "Test>Something",
                    })
                    retest.expect(action.remove("Test>Something")).to.equal({ "Test>Something" })
                end
                -- Can't test for crashes in Lua, so this is just a smoke test.
                retest.expect(true).to.be.truthy()
            end)
        end)

        retest.describe('associate_hotkey', function()
            retest.after(function()
                action.remove("Test > *")
            end)

            retest.it('errors_when_path_is_nil', function()
                local func = function()
                    action.associate_hotkey(nil, {})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_is_not_string', function()
                local func = function()
                    action.associate_hotkey({}, {})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_hotkey_is_nil', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                    })
                    action.associate_hotkey("Test > Something", nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_hotkey_is_not_table', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                    })
                    action.associate_hotkey("Test > Something", 5)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_overwrite_existing_is_not_boolean', function()
                local func = function()
                    action.add({
                        path = "Test > Something",
                    })
                    action.associate_hotkey("Test > Something", 5, 5)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_false_when_action_doesnt_exist', function()
                local result = action.associate_hotkey("Test > Something", {})
                retest.expect(result).to.equal(false)
            end)
            retest.it('returns_false_when_path_isnt_fully_qualified', function()
                action.add({
                    path = "Test > Something",
                })
                local result = action.associate_hotkey("Test > *", {})
                retest.expect(result).to.equal(false)
            end)
            retest.it('works_when_parameters_valid', function()
                action.add({
                    path = "Test > Something",
                })
                local result = action.associate_hotkey("Test > Something", { key = Mupen.VKeycodes.VK_TAB }, true)
                retest.expect(result).to.be.truthy()
            end)
        end)

        retest.describe('batch_work', function()
            retest.it('doesnt_error', function()
                action.begin_batch_work()
                action.end_batch_work()
            end)
        end)

        retest.describe('notify_enabled_changed', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.notify_enabled_changed(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.notify_enabled_changed({})
                end
                retest.expect(func).to.fail()
            end)
            -- A test like "calls_callback_on_affected_actions" is not applicable because we can't know when the callback will be called.
        end)

        retest.describe('notify_active_changed', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.notify_active_changed(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.notify_active_changed({})
                end
                retest.expect(func).to.fail()
            end)
        end)

        retest.describe('notify_display_name_changed', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.notify_display_name_changed(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.notify_display_name_changed({})
                end
                retest.expect(func).to.fail()
            end)
        end)

        retest.describe('get_display_name', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.get_display_name(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.get_display_name({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_correct_name_when_no_action_matched', function()
                local name = action.get_display_name("Test >    Something")
                retest.expect(name).to.equal("Something")
            end)
            retest.it('returns_correct_name_when_no_action_matched_with_separator', function()
                local name = action.get_display_name("Test >    Something ---")
                retest.expect(name).to.equal("Something")
            end)
            retest.it('returns_correct_name_when_action_matched', function()
                action.add({
                    path = "Test > Something",
                })
                local name = action.get_display_name("Test >    Something")
                retest.expect(name).to.equal("Something")
            end)
            retest.it('returns_correct_name_when_action_matched_with_separator', function()
                action.add({
                    path = "Test > Something---",
                })
                local name = action.get_display_name("Test >    Something ---")
                retest.expect(name).to.equal("Something")
            end)
            retest.it('uses_display_name', function()
                action.add({
                    path = "Test > Something",
                    get_display_name = function()
                        return "Hi!"
                    end
                })
                local name = action.get_display_name("Test >    Something")
                retest.expect(name).to.equal("Hi!")
            end)
            retest.it('doesnt_use_display_name_when_ignore_override_true', function()
                action.add({
                    path = "Test > Something",
                    get_display_name = function()
                        return "Hi!"
                    end
                })
                local name = action.get_display_name("Test >    Something", true)
                retest.expect(name).to.equal("Something")
            end)
            retest.it('doesnt_use_display_name_when_ignore_override_true_with_separator', function()
                action.add({
                    path = "Test > Something ---",
                    get_display_name = function()
                        return "Hi!"
                    end
                })
                local name = action.get_display_name("Test >    Something---", true)
                retest.expect(name).to.equal("Something")
            end)
        end)

        retest.describe('get_enabled', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_enabled(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_enabled({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_false_when_action_doesnt_exist', function()
                local result = action.get_enabled("Test > Something")
                retest.expect(result).to.equal(false)
            end)
            retest.it('returns_enabled_state', function()
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
                retest.expect(action.get_enabled("Test > A")).to.equal(true)
                retest.expect(action.get_enabled("Test > B")).to.equal(false)
            end)
        end)

        retest.describe('get_active', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_active(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_active({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_false_when_action_doesnt_exist', function()
                local result = action.get_active("Test > Something")
                retest.expect(result).to.equal(false)
            end)
            retest.it('returns_active_state', function()
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
                retest.expect(action.get_active("Test > A")).to.equal(true)
                retest.expect(action.get_active("Test > B")).to.equal(false)
            end)
        end)

        retest.describe('get_activatability', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_activatability(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_activatability({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_false_when_action_doesnt_exist', function()
                local result = action.get_activatability("Test > Something")
                retest.expect(result).to.equal(false)
            end)
            retest.it('returns_true_when_get_active_callback_present', function()
                action.add({
                    path = "Test > Something",
                    get_active = function()
                        return true
                    end
                })
                local result = action.get_activatability("Test > Something")
                retest.expect(result).to.equal(true)
            end)
            retest.it('returns_false_when_get_active_callback_absent', function()
                action.add({
                    path = "Test > Something",
                })
                local result = action.get_activatability("Test > Something")
                retest.expect(result).to.equal(false)
            end)
        end)

        retest.describe('get_params', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_path_is_nil', function()
                local func = function()
                    action.get_params(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_is_table', function()
                local func = function()
                    action.get_params({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_empty_list_when_action_doesnt_exist', function()
                local result = action.get_params("Test > Something")
                retest.expect(result).to.equal({})
            end)
            retest.it('returns_params', function()
                local params = {
                    {
                        key = "param1",
                        name = "Parameter 1",
                        validator = function() end
                    }
                }
                action.add({
                    path = "Test > Something",
                    params = params
                })
                local result = action.get_params("Test > Something")
                retest.expect(result).to.equal(params)
            end)
            retest.it('returns_params_across_normalization_boundary', function()
                local params = {
                    {
                        key = "param1",
                        name = "Parameter 1",
                        validator = function() end
                    }
                }
                action.add({
                    path = "Test >  Something  ",
                    params = params
                })
                local result = action.get_params("Test>Something")
                retest.expect(result).to.equal(params)
            end)
        end)

        retest.describe('get_actions_matching_filter', function()
            retest.after(function()
                action.remove("Test > *")
            end)
            retest.it('errors_when_filter_is_nil', function()
                local func = function()
                    action.get_actions_matching_filter(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_filter_is_not_string', function()
                local func = function()
                    action.get_actions_matching_filter({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('matches_even_with_whitespace_and_extra_separators', function()
                action.add({
                    path = "Test>X",
                })

                local result = action.get_actions_matching_filter(" Test  >  X ")
                retest.expect(result).to.equal({ "Test>X" })
            end)
            retest.it('wildcard_requires_additional_segments', function()
                action.add({
                    path = "Test>X",
                })

                local result = action.get_actions_matching_filter("Test > *")
                retest.expect(result).to.equal({ "Test>X" })

                result = action.get_actions_matching_filter("Test > X > *")
                retest.expect(result).to.equal({})
            end)
            retest.it('returns_empty_for_empty_filter', function()
                local result = action.get_actions_matching_filter("")
                retest.expect(result).to.equal({})
            end)
            retest.it('does_not_match_partial_paths_without_wildcard', function()
                action.add({
                    path = "Test>X",
                })

                local result = action.get_actions_matching_filter("Test")
                retest.expect(result).to.equal({})
            end)
            retest.it('returns_correct_actions_wildcard_special_case', function()
                local result = action.get_actions_matching_filter("*")
                -- Flaky: we can't guarantee the number of actions, but we can check that there are roughly enough to be the entire built-in menu.
                retest.expect(#result > 50).to.be.truthy()
            end)
            retest.it('returns_correct_actions', function()
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
                retest.expect(result).to.equal({})

                result = action.get_actions_matching_filter("Test >    *")
                retest.expect(result).to.equal(actions)

                result = action.get_actions_matching_filter("Test  > Something---")
                retest.expect(result).to.equal({})

                result = action.get_actions_matching_filter("Test>Something---> *")
                retest.expect(result).to.equal({
                    "Test>Something--->A"
                })
            end)
        end)

        retest.describe('invoke', function()
            retest.after(function()
                action.remove("Test > *")
            end)

            retest.it('errors_when_path_is_nil', function()
                local func = function()
                    action.invoke(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_path_is_not_string', function()
                local func = function()
                    action.invoke({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('calls_on_press', function()
                local called = false
                action.add({
                    path = "Test > Something",
                    on_press = function()
                        called = true
                    end
                })

                action.invoke("Test > Something")
                retest.expect(called).to.be.truthy()
            end)
            retest.it('calls_on_release', function()
                local called = false
                action.add({
                    path = "Test > Something",
                    on_release = function()
                        called = true
                    end
                })

                action.invoke("Test > Something", true)
                retest.expect(called).to.be.truthy()
            end)
            retest.it('calls_on_release_when_pressing_again_while_pressed', function()
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
                retest.expect(down).to.equal(1)
                retest.expect(up).to.equal(0)

                action.invoke("Test > Something")
                retest.expect(down).to.equal(1)
                retest.expect(up).to.equal(1)
            end)
            retest.it('doesnt_call_release_when_pressing_again_while_pressed_if_release_on_repress_is_false', function()
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
                retest.expect(down).to.equal(1)
                retest.expect(up).to.equal(0)

                action.invoke("Test > Something", false, false)
                retest.expect(down).to.equal(2)
                retest.expect(up).to.equal(0)
            end)
            retest.it('doesnt_call_onpress_when_parameter_count_mismatched', function()
                local called = false
                action.add({
                    path = "Test > Something",
                    params = {
                        {
                            key = "param1",
                            name = "Parameter 1",
                            validator = function() end
                        },
                    },
                    on_press = function()
                        called = true
                    end
                })

                action.invoke("Test > Something", nil, nil, {})

                retest.expect(called).to.equal(false)
            end)
            retest.it('doesnt_call_onpress_when_validation_fails', function()
                local called = false
                action.add({
                    path = "Test > Something",
                    params = {
                        {
                            key = "param1",
                            name = "Parameter 1",
                            validator = function() return "error" end
                        },
                    },
                    on_press = function()
                        called = true
                    end
                })

                action.invoke("Test > Something", nil, nil, { param1 = "aaa" })

                retest.expect(called).to.equal(false)
            end)
            retest.it('calls_onpress_with_correct_params', function()
                local received_params
                action.add({
                    path = "Test > Something",
                    params = {
                        {
                            key = "param1",
                            name = "Parameter 1",
                            validator = function() end
                        },
                        {
                            key = "param2",
                            name = "Parameter 2",
                            validator = function() end
                        },
                    },
                    on_press = function(params)
                        received_params = params
                    end
                })

                action.invoke("Test > Something", nil, nil, { param1 = "aaa", param2 = "" })

                retest.expect(received_params).to.equal({ param1 = "aaa", param2 = "" })
            end)
        end)

        retest.describe('lock_hotkeys', function()
            retest.it('errors_when_lock_nil', function()
                local func = function()
                    action.lock_hotkeys(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_lock_not_bool', function()
                local func = function()
                    action.lock_hotkeys({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('succeeds_when_lock_bool', function()
                local func = function()
                    action.lock_hotkeys(false)
                end
                retest.expect(func).to_not.fail()
            end)
        end)

        retest.describe('get_hotkeys_locked', function()
            retest.it('returns_locked_state', function()
                action.lock_hotkeys(true)
                retest.expect(action.get_hotkeys_locked()).to.equal(true)
                action.lock_hotkeys(false)
                retest.expect(action.get_hotkeys_locked()).to.equal(false)
            end)
        end)
    end)

    retest.describe('hotkeys', function()
        retest.describe('prompt', function()
            retest.it('errors_when_caption_nil', function()
                local func = function()
                    hotkeys.prompt(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_when_caption_not_string', function()
                local func = function()
                    hotkeys.prompt({})
                end
                retest.expect(func).to.fail()
            end)
        end)
    end)

    retest.describe('clipboard', function()
        retest.describe('get', function()
            retest.it('errors_if_type_nil', function()
                clipboard.clear()
                local func = function()
                    clipboard.get(nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_if_type_not_string', function()
                clipboard.clear()
                local func = function()
                    clipboard.get({})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_if_type_doesnt_exist', function()
                clipboard.clear()
                local func = function()
                    clipboard.get("blah blah blah")
                end
                retest.expect(func).to.fail()
            end)
            retest.it('returns_nil_if_clipboard_empty', function()
                clipboard.clear()
                retest.expect(clipboard.get("text")).to.equal(nil)
            end)
            retest.it('returns_clipboard_value', function()
                clipboard.set("text", "test")
                retest.expect(clipboard.get("text")).to.equal("test")
            end)
        end)
        retest.describe('get_content_type', function()
            retest.it('returns_nil_if_empty', function()
                clipboard.clear()
                retest.expect(clipboard.get_content_type()).to.equal(nil)
            end)
            retest.it('returns_content_type', function()
                clipboard.set("text", "test")
                retest.expect(clipboard.get_content_type()).to.equal("text")
            end)
        end)
        retest.describe('set', function()
            retest.it('errors_if_type_nil', function()
                clipboard.clear()
                local func = function()
                    clipboard.set(nil, "test")
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_if_type_not_string', function()
                clipboard.clear()
                local func = function()
                    clipboard.set({}, "test")
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_if_type_doesnt_exist', function()
                clipboard.clear()
                local func = function()
                    clipboard.set("blah blah blah", "test")
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_if_value_nil', function()
                clipboard.clear()
                local func = function()
                    clipboard.set("text", nil)
                end
                retest.expect(func).to.fail()
            end)
            retest.it('errors_if_value_not_string', function()
                clipboard.clear()
                local func = function()
                    clipboard.set("text", {})
                end
                retest.expect(func).to.fail()
            end)
            retest.it('get_returns_set_value_after', function()
                clipboard.set("text", "test")
                retest.expect(clipboard.get("text")).to.equal("test")
            end)
            retest.it('overwrites_existing_text', function()
                clipboard.set("text", "test")
                retest.expect(clipboard.get("text")).to.equal("test")

                clipboard.set("text", "test2")
                retest.expect(clipboard.get("text")).to.equal("test2")
            end)
        end)
        retest.describe('clear', function()
            retest.it('get_returns_nil_after', function()
                clipboard.clear()
                retest.expect(clipboard.get("text")).to.equal(nil)
            end)
        end)
    end)
end)

retest.run()

local report = retest.report()
emu.console(report)
