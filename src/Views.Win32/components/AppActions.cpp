#include "stdafx.h"
#include <DialogService.h>
#include <ActionManager.h>
#include <ThreadPool.h>
#include <components/AppActions.h>
#include <components/ConfigDialog.h>
#include <components/FilePicker.h>

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
        ThreadPool::submit_task([path] {
            const auto result = g_core_ctx->vr_start_rom(path);
            show_error_dialog_for_result(result);
        });
    }
}

static void show_settings()
{
    BetterEmulationLock lock;
    ConfigDialog::show_app_settings();
}

static void add_and_associate_with_default_hotkey(const std::wstring& path, const Hotkey::t_hotkey& default_hotkey, const std::function<void()>& down_callback, const std::function<void()>& up_callback = {})
{
    bool success = ActionManager::add(path, down_callback, up_callback);
    runtime_assert(success, std::format(L"Failed to add action for path '{}'.", path));

    success = ActionManager::associate_hotkey(path, g_config.hotkeys.contains(path) ? g_config.hotkeys[path] : default_hotkey);
    runtime_assert(success, std::format(L"Failed to associate hotkey for path '{}'.", path));
}

void AppActions::add()
{
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Load ROM...", {.key = 'O', .ctrl = true}, load_rom);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Close ROM", {.key = 'W', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Reset ROM", {.key = 'R', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Refresh ROM List ---", {.key = VK_F5, .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Recent ROMs > Reset", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Recent ROMs > Freeze ---", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Load Latest ROM ---", {.key = 'O', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Exit", {.key = VK_F4, .alt = true}, stub);

    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Pause", {.key = VK_PAUSE}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Speed Down", {.key = VK_OEM_MINUS}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Speed Up", {.key = VK_OEM_PLUS}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Reset Speed", {.key = VK_OEM_PLUS, .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Frame Advance", {.key = VK_OEM_5}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance", {.key = VK_OEM_5, .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Take Screenshot ---", {.key = VK_F12}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Save State", {.key = 'I'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Load State", {.key = 'P'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Save State As...", {.key = 'N', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Load State As...", {.key = 'M', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Undo Load State As ---", {.key = 'Z', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance +1", {.key = 'E', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance -1", {.key = 'Q', .ctrl = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Multi-Frame Advance Reset ---", {.key = 'E', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 1", {.key = '1'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 2", {.key = '2'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 3", {.key = '3'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 4", {.key = '4'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 5", {.key = '5'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 6", {.key = '6'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 7", {.key = '7'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 8", {.key = '8'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 9", {.key = '9'}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Emulation > Current Save State > Slot 10", {.key = '0'}, stub);

    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Full Screen", {.key = VK_RETURN, .alt = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > Video Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > Audio Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > Input Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Plugin Settings --- > RSP Settings", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Show Statusbar ---", {.key = 'S', .alt = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Settings...", {.key = 'S', .ctrl = true}, show_settings);

    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Start Movie Recording", {.key = 'R', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Start Movie Playback ---", {.key = 'P', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Stop Movie", {.key = 'C', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Create Movie Backup ---", {.key = 'B', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Recent Movies --- > Reset", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Recent Movies --- > Freeze ---", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Load Latest Movie ---", {.key = 'T', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Loop Movie Playback", {.key = 'L', .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Read-Only", {.key = 'R', .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Movie > Wait at Movie End", {}, stub);
    
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Show RAM Start", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Show Statistics", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Start Trace Logger...", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Core Debugger", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Run", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Cheats", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Seek To...", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Piano Roll ---", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Video Capture > Start Capture...", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Video Capture > Start from Preset...", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Utilities > Video Capture > Stop Capture", {}, stub);
    
    add_and_associate_with_default_hotkey(L"Mupen64 > Help > Check for Updates", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Help > About Mupen64", {}, stub);
    
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > New Instance...", {.key = 'N', .ctrl = true }, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Recent Scripts --- > Reset", {}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Recent Scripts --- > Freeze ---", { }, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Load Latest Script ---", {.key = 'K', .ctrl = true, .shift = true }, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Lua Script > Close All", {.key = 'W', .ctrl = true, .shift = true }, stub);
}
