/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdafx.h>
#include <Config.h>
#include <features/FilePicker.h>

#define FAILSAFE(operation) \
    if (FAILED(operation))  \
    goto cleanUp

static std::wstring fix_filter(const std::wstring& filter)
{
    const std::wstring description = L"Allowed Files (" + filter + L")";
    std::wstring result = description + L'\0' + filter + L'\0';
    return result;
}

static std::wstring get_default_extension(const std::wstring& filter)
{
    const auto wildcards = split_string(filter, L";");

    if (wildcards.empty())
        return L"";

    const auto& first_wildcard = wildcards[0];

    return first_wildcard.substr(2);
}

std::filesystem::path FilePicker::show_open_dialog(const std::wstring& id, HWND hwnd, const std::wstring& filter)
{
    std::filesystem::path restored_path = g_config.persistent_folder_paths.contains(id)
    ? g_config.persistent_folder_paths[id]
    : get_desktop_path();

    g_view_logger->info(L"file dialog {}: {}\n", id, restored_path.wstring());

    const auto fixed_filter = fix_filter(filter);

    wchar_t out_path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = std::size(out_path);
    ofn.lpstrFilter = fixed_filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = restored_path.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING;

    if (GetOpenFileName(&ofn))
    {
        g_config.persistent_folder_paths[id] = ofn.lpstrFile;
        return g_config.persistent_folder_paths[id];
    }

    return {};
}

std::filesystem::path FilePicker::show_save_dialog(const std::wstring& id, HWND hwnd, const std::wstring& filter)
{
    std::filesystem::path restored_path = g_config.persistent_folder_paths.contains(id)
    ? g_config.persistent_folder_paths[id]
    : get_desktop_path();

    g_view_logger->info(L"file dialog {}: {}\n", id, restored_path.wstring());

    const auto fixed_filter = fix_filter(filter);
    const auto default_extension = get_default_extension(filter);

    wchar_t out_path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = std::size(out_path);
    ofn.lpstrFilter = fixed_filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = restored_path.c_str();
    ofn.lpstrDefExt = default_extension.c_str();
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXTENSIONDIFFERENT;

    if (GetSaveFileName(&ofn))
    {
        g_config.persistent_folder_paths[id] = ofn.lpstrFile;
        return g_config.persistent_folder_paths[id];
    }

    return {};
}

std::filesystem::path FilePicker::show_folder_dialog(const std::wstring& id, HWND hwnd)
{
    std::filesystem::path restored_path = g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    COMInitializer com_initializer;
    std::wstring final_path;
    IFileDialog* pfd;

    g_view_logger->info(L"folder dialog {}: {}\n", id, restored_path.wstring());

    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
    {
        PIDLIST_ABSOLUTE pidl;

        HRESULT hresult = SHParseDisplayName(restored_path.c_str(), nullptr, &pidl, SFGAO_FOLDER, nullptr);
        if (SUCCEEDED(hresult))
        {
            IShellItem* psi;
            hresult = SHCreateShellItem(nullptr, nullptr, pidl, &psi);
            if (SUCCEEDED(hresult))
            {
                pfd->SetFolder(psi);
            }
            ILFree(pidl);
        }

        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
        {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);
        }

        if (SUCCEEDED(pfd->Show(hwnd)))
        {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi)))
            {
                WCHAR* tmp;
                if (SUCCEEDED(
                    psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &tmp)))
                {
                    final_path = tmp;
                }
                psi->Release();
            }
        }
        pfd->Release();
    }

    if (final_path.size() > 1)
    {
        // probably valid path, store it
        g_config.persistent_folder_paths[id] = final_path;
    }

    return final_path;
}
