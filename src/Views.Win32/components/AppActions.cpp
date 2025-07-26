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

static void add_and_associate_hotkey_from_config(const std::wstring& path, const std::function<void()>& down_callback, const std::function<void()>& up_callback = {})
{
    ActionManager::add_and_associate_hotkey(path, g_config.hotkeys[path], down_callback, up_callback);
}

void AppActions::add()
{
    add_and_associate_hotkey_from_config(L"Mupen64 > File > Load ROM...", load_rom);
    add_and_associate_hotkey_from_config(L"Mupen64 > File > Close ROM", stub);
    add_and_associate_hotkey_from_config(L"Mupen64 > File > Reset ROM", stub);
    add_and_associate_hotkey_from_config(L"Mupen64 > File > Load Latest ROM", stub);
    add_and_associate_hotkey_from_config(L"Mupen64 > File > Exit", stub);
    add_and_associate_hotkey_from_config(L"Mupen64 > Options > Settings...", show_settings);
}
