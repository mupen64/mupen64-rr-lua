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

static void add_and_associate_with_default_hotkey(const std::wstring& path, const t_hotkey& default_hotkey, const std::function<void()>& down_callback, const std::function<void()>& up_callback = {})
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
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Load Latest ROM", {.key = 'O', .ctrl = true, .shift = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > File > Exit", {.key = VK_F4, .alt = true}, stub);
    add_and_associate_with_default_hotkey(L"Mupen64 > Options > Settings...", {.key = 'S', .ctrl = true}, show_settings);
}
