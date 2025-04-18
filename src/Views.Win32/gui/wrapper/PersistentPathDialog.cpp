/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <gui/wrapper/PersistentPathDialog.h>
#define FAILSAFE(operation) \
    if (FAILED(operation))  \
    goto cleanUp

std::wstring show_persistent_open_dialog(const std::wstring& id, HWND hwnd, const std::wstring& filter)
{
    std::wstring restored_path = g_config.persistent_folder_paths.contains(id)
    ? g_config.persistent_folder_paths[id]
    : get_desktop_path();
    g_view_logger->info(L"file dialog {}: {}\n", id.c_str(), restored_path);

    wchar_t out_path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = std::size(out_path);
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = restored_path.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING;

    if (GetOpenFileName(&ofn))
    {
        g_config.persistent_folder_paths[id] = ofn.lpstrFile;
        return g_config.persistent_folder_paths[id];
    }

    return L"";
}

std::wstring show_persistent_save_dialog(const std::wstring& id, HWND hwnd, const std::wstring& filter)
{
    std::wstring restored_path = g_config.persistent_folder_paths.contains(id)
    ? g_config.persistent_folder_paths[id]
    : get_desktop_path();
    g_view_logger->info(L"file dialog {}: {}\n", id.c_str(), restored_path);

    wchar_t out_path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = std::size(out_path);
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = restored_path.c_str();
    ofn.Flags = OFN_CREATEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING;

    if (GetOpenFileName(&ofn))
    {
        g_config.persistent_folder_paths[id] = ofn.lpstrFile;
        return g_config.persistent_folder_paths[id];
    }

    return L"";
}

std::wstring show_persistent_folder_dialog(const std::wstring& id, HWND hwnd)
{
    std::filesystem::path restored_path = g_config.persistent_folder_paths.contains(id)
    ? g_config.persistent_folder_paths[id]
    : get_desktop_path();

    COMInitializer com_initializer;

    BROWSEINFO bi{};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = _T("Select a folder");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = [](const HWND hwnd, const UINT msg, LPARAM, const LPARAM lpdata) -> int {
        if (msg == BFFM_INITIALIZED)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpdata);
        }
        return 0;
    };
    bi.lParam = (LPARAM)restored_path.wstring().c_str();

    bool success = false;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);

    if (!pidl)
    {
        return L"";
    }

    wchar_t path[MAX_PATH];
    if (SHGetPathFromIDList(pidl, path))
    {
        g_config.persistent_folder_paths[id] = path;
        CoTaskMemFree(pidl);
        success = true;
    }
    CoTaskMemFree(pidl);

    return success ? path : L"";
}
