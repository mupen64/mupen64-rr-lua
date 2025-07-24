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

void AppActions::add()
{
    ActionManager::add_and_associate_hotkey(L"Mupen64 > File > Load ROM...", {'O', true}, load_rom);
    ActionManager::add_and_associate_hotkey(L"Mupen64 > File > Close ROM", {'W', true}, stub);
    ActionManager::add_and_associate_hotkey(L"Mupen64 > File > Reset ROM", {'R', true}, stub);
    ActionManager::add_and_associate_hotkey(L"Mupen64 > File > Load Latest ROM", {'O', true, true}, stub);
    ActionManager::add_and_associate_hotkey(L"Mupen64 > File > Exit", {VK_F4, true}, stub);

    ActionManager::add_and_associate_hotkey(L"Mupen64 > Options > Settings...", {'S', true}, show_settings);
}
