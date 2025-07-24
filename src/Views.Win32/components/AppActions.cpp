#include "stdafx.h"
#include <components/AppActions.h>
#include <ActionManager.h>
#include <resource.h>
#include <components/FilePicker.h>
#include <ThreadPool.h>

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

void AppActions::add()
{
    ActionManager::add(L"Mupen64 > File > Load ROM...", load_rom);
    ActionManager::associate_hotkey(L"Mupen64 > File > Load ROM...", {'O', true});
}
