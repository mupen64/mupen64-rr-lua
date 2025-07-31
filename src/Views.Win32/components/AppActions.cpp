#include "stdafx.h"
#include "capture/EncodingManager.h"

#include <DialogService.h>
#include <ActionManager.h>
#include <ThreadPool.h>
#include <components/AppActions.h>
#include <components/ConfigDialog.h>
#include <components/FilePicker.h>

#pragma region Action Callbacks

static void stub()
{
    DialogService::show_dialog(L"ActionManager::stub", L"Stub", fsvc_error);
}

static void load_rom()
{
    BetterEmulationLock lock;

    const auto path = FilePicker::show_open_dialog(L"o_rom", g_main_hwnd, L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap");

    if (!path.empty())
    {
        ThreadPool::submit_task([=] {
            const auto result = g_core_ctx->vr_start_rom(path);
            show_error_dialog_for_result(result);
        });
    }
}

static void load_recent_rom(size_t i)
{
    if (g_config.recent_rom_paths.size() <= i)
    {
        return;
    }

    const auto path = g_config.recent_rom_paths[i];

    ThreadPool::submit_task([=] {
        const auto result = g_core_ctx->vr_start_rom(path);
        show_error_dialog_for_result(result);
    });
}

static void load_recent_movie(size_t i)
{
    if (g_config.recent_movie_paths.size() <= i)
    {
        return;
    }

    runtime_assert(false, L"TODO");
}

static void load_recent_script(size_t i)
{
    if (g_config.recent_lua_script_paths.size() <= i)
    {
        return;
    }

    runtime_assert(false, L"TODO");
}

static void close_rom()
{
    if (!confirm_user_exit())
        return;

    ThreadPool::submit_task([] {
        const auto result = g_core_ctx->vr_close_rom(true);
        show_error_dialog_for_result(result);
    },
                            ASYNC_KEY_CLOSE_ROM);
}

static void show_settings()
{
    BetterEmulationLock lock;
    ConfigDialog::show_app_settings();
}

#pragma endregion

#pragma region Enabled Getters

static bool enable_when_emu_launched()
{
    return g_core_ctx->vr_get_launched();
}

static bool enable_when_emu_launched_and_vcr_active()
{
    return g_core_ctx->vr_get_launched() && g_core_ctx->vcr_get_task() != task_idle;
}

static bool enable_when_emu_launched_and_capturing()
{
    return g_core_ctx->vr_get_launched() && EncodingManager::is_capturing();
}

static bool enable_when_emu_launched_and_core_is_pure_interpreter()
{
    return g_core_ctx->vr_get_launched() && g_config.core.core_type == 2;
}

static bool always_enabled()
{
    return true;
}

#pragma endregion

static void add_and_associate_with_default_hotkey(const std::wstring& path, const Hotkey::t_hotkey& default_hotkey, const std::function<void()>& down_callback, const std::function<bool()>& get_enabled = {}, const std::function<bool()>& get_active = {}, const std::function<std::wstring()>& get_real_name = {})
{
    bool success = ActionManager::add({
    .path = path,
    .down_callback = down_callback,
    .get_enabled = get_enabled ? get_enabled : [] {
        return true;
    },
    .get_active = get_active ? get_active : [] {
        return false;
    },
    .get_real_name = get_real_name,
    });
    runtime_assert(success, std::format(L"Failed to add action for path '{}'.", path));

    success = ActionManager::associate_hotkey(path, g_config.hotkeys.contains(path) ? g_config.hotkeys[path] : default_hotkey);
    runtime_assert(success, std::format(L"Failed to associate hotkey for path '{}'.", path));
}

static void generate_path_recent_menu(const std::wstring& base_path, const std::vector<std::wstring>* paths, const std::function<void(size_t)>& callback)
{
    for (size_t i = 0; i < 10; ++i)
    {
        const auto get_real_name = [=] -> std::wstring {
            if (paths->size() > i)
            {
                return std::filesystem::path(paths->at(i)).filename();
            }
            return L"(nothing)";
        };

        const auto path = std::format(L"{} > Item #{}", base_path, i + 1);
        
        add_and_associate_with_default_hotkey(path, {}, [=] {
            callback(i);
        },
                                              {},
                                              {},
                                              get_real_name);
    }
}

void AppActions::add()
{
    ActionManager::begin_batch_work();
    
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Load ROM...", {.key = 'O', .ctrl = true}, load_rom);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Close ROM", {.key = 'W', .ctrl = true}, close_rom, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Reset ROM", {.key = 'R', .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Refresh ROM List ---", {.key = VK_F5, .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Recent ROMs > Reset", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Recent ROMs > Freeze ---", {}, stub, always_enabled, [] {
        return g_config.is_recent_rom_paths_frozen;
    });
    generate_path_recent_menu(L"Mupen64 > File > Recent ROMs", &g_config.recent_rom_paths, load_recent_rom);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Load Latest ROM ---", {.key = 'O', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Exit", {.key = VK_F4, .alt = true}, stub);

    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Pause", {.key = VK_PAUSE}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Speed Down", {.key = VK_OEM_MINUS}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Speed Up", {.key = VK_OEM_PLUS}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Reset Speed", {.key = VK_OEM_PLUS, .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Frame Advance", {.key = VK_OEM_5}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance", {.key = VK_OEM_5, .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Take Screenshot ---", {.key = VK_F12}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Save State", {.key = 'I'}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Load State", {.key = 'P'}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Save State As...", {.key = 'N', .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Load State As...", {.key = 'M', .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Undo Load State As ---", {.key = 'Z', .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance +1", {.key = 'E', .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance -1", {.key = 'Q', .ctrl = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance Reset ---", {.key = 'E', .ctrl = true, .shift = true}, stub, enable_when_emu_launched);
    for (size_t i = 0; i < 10; ++i)
    {
        const int32_t key = i < 9 ? '1' + i : '0';

        const auto get_active = [=] {
            return g_config.st_slot == i;
        };

        add_and_associate_with_default_hotkey(std::format(L"Mupen64 > Emulation > Current Save State > Slot {}", i + 1), {.key = key}, stub, enable_when_emu_launched, get_active);
    }


    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Full Screen ---", {.key = VK_RETURN, .alt = true}, stub, enable_when_emu_launched, [] {
        // FIXME
        return false;
    });
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > Video Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > Audio Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > Input Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > RSP Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Show Statusbar ---", {.key = 'S', .alt = true}, stub, always_enabled, [] {
        return g_config.is_statusbar_enabled;
    });
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Settings...", {.key = 'S', .ctrl = true}, show_settings);

    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Start Movie Recording", {.key = 'R', .ctrl = true, .shift = true}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Start Movie Playback ---", {.key = 'P', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Stop Movie", {.key = 'C', .ctrl = true, .shift = true}, stub, enable_when_emu_launched_and_vcr_active);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Create Movie Backup ---", {.key = 'B', .ctrl = true, .shift = true}, stub, enable_when_emu_launched_and_vcr_active);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Recent Movies > Reset", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Recent Movies > Freeze ---", {}, stub, always_enabled, [] {
        return g_config.is_recent_movie_paths_frozen;
    });
    generate_path_recent_menu(L"Mupen64 > Movie > Recent Movies", &g_config.recent_movie_paths, load_recent_movie);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Load Latest Movie ---", {.key = 'T', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Loop Movie Playback", {.key = 'L', .shift = true}, stub, always_enabled, [] {
        return g_config.core.is_movie_loop_enabled;
    });
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Read-Only", {.key = 'R', .shift = true}, stub, always_enabled, [] {
        return g_config.core.vcr_readonly;
    });
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Wait at Movie End", {}, stub, always_enabled, [] {
        return g_config.core.wait_at_movie_end;
    });

    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Show RAM Start", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Show Statistics", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Start Trace Logger...", {}, stub, enable_when_emu_launched_and_core_is_pure_interpreter);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Core Debugger", {}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Run", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Cheats", {}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Seek To...", {}, stub, enable_when_emu_launched_and_vcr_active);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Piano Roll ---", {}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Video Capture > Start Capture...", {}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Video Capture > Start from Preset...", {}, stub, enable_when_emu_launched);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Video Capture > Stop Capture", {}, stub, enable_when_emu_launched_and_capturing);

    add_and_associate_with_default_hotkey(L"Mupen64 > Help > Check for Updates", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Help > About Mupen64", {}, stub);

    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > New Instance... ---", {.key = 'N', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Recent Scripts > Reset", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Recent Scripts > Freeze ---", {}, stub, always_enabled, [] {
        return g_config.is_recent_scripts_frozen;
    });
    generate_path_recent_menu(L"Mupen64 > Lua Script > Recent Scripts", &g_config.recent_lua_script_paths, load_recent_script);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Load Latest Script ---", {.key = 'K', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Close All", {.key = 'W', .ctrl = true, .shift = true}, stub);

    ActionManager::end_batch_work();
}
