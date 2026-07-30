/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <filesystem>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include <wrl/client.h>
#elif defined(__linux__)
#error "Don't include this file on Linux"
#endif

namespace WinFilePicker
{
inline std::string build_filter(std::string_view filter)
{
    auto description = std::format("Allowed Files ({})", filter);
    return description + '\0' + std::string(filter) + '\0';
}
inline std::string get_default_extension(std::string_view filter)
{
    const auto wildcards = StrUtils::split_string(filter, ";");

    if (wildcards.empty()) return "";

    const auto first_wildcard = *wildcards.begin();
    return std::string(first_wildcard.substr(2));
}

/**
 * \brief Shows a file selection dialog.
 * \param hwnd The dialog owner's window handle, or NULL.
 * \param filter A collection of valid extension patterns separated by semicolons. Example: `*.png;*.jpg`.
 * \param initial_path The initial path to display in the dialog.
 * \return The chosen file's path, or an empty path if the dialog was cancelled.
 */
inline std::filesystem::path show_open_dialog(HWND hwnd, std::string_view filter,
                                              const std::filesystem::path &initial_path = {})
{
    const auto built_filter = IOUtils::to_wide_string(build_filter(filter));
    const auto initial_dir = initial_path.wstring();

    wchar_t out_path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = std::size(out_path);
    ofn.lpstrFilter = built_filter.c_str();
    if (!initial_dir.empty()) ofn.lpstrInitialDir = initial_dir.c_str();
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING;

    wchar_t cwd[MAX_PATH]{};
    GetCurrentDirectory(std::size(cwd), cwd);

    const auto result = GetOpenFileName(&ofn);

    SetCurrentDirectory(cwd);

    if (result) return std::filesystem::path(ofn.lpstrFile);
    return {};
}

/**
 * \brief Shows a file save dialog.
 * \param hwnd The dialog owner's window handle, or NULL.
 * \param filter A collection of valid extension patterns separated by semicolons. Example: `*.png;*.jpg`.
 * \param initial_path The initial path to display in the dialog.
 * \return The chosen file's path, or an empty path if the dialog was cancelled.
 */
inline std::filesystem::path show_save_dialog(HWND hwnd, std::string_view filter,
                                              const std::filesystem::path &initial_path = {})
{
    const auto built_filter = IOUtils::to_wide_string(build_filter(filter));
    const auto default_extension = IOUtils::to_wide_string(get_default_extension(filter));
    const auto initial_dir = initial_path.wstring();

    wchar_t out_path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = std::size(out_path);
    ofn.lpstrFilter = built_filter.c_str();
    if (!initial_dir.empty()) ofn.lpstrInitialDir = initial_dir.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = default_extension.c_str();
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXTENSIONDIFFERENT;

    wchar_t cwd[MAX_PATH]{};
    GetCurrentDirectory(std::size(cwd), cwd);

    const auto result = GetSaveFileName(&ofn);

    SetCurrentDirectory(cwd);

    if (result) return std::filesystem::path(ofn.lpstrFile);
    return {};
}

/**
 * \brief Shows a folder selection dialog.
 * \param hwnd The dialog owner's window handle, or NULL.
 * \param initial_path The initial path to display in the dialog.
 * \return The chosen folder's path, or an empty path if the dialog was cancelled.
 */
inline std::filesystem::path show_folder_dialog(HWND hwnd, const std::filesystem::path &initial_path = {})
{
    using Microsoft::WRL::ComPtr;

    const auto initial_dir = initial_path.wstring();

    std::filesystem::path result_path;

    ComPtr<IFileDialog> dialog;
    HRESULT hr =
        CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.GetAddressOf()));

    if (SUCCEEDED(hr))
    {
        PIDLIST_ABSOLUTE pidl = nullptr;

        hr = SHParseDisplayName(initial_dir.c_str(), nullptr, &pidl, SFGAO_FOLDER, nullptr);

        if (SUCCEEDED(hr))
        {
            ComPtr<IShellItem> folder;

            hr = SHCreateShellItem(nullptr, nullptr, pidl, folder.GetAddressOf());

            if (SUCCEEDED(hr))
            {
                dialog->SetFolder(folder.Get());
            }

            ILFree(pidl);
        }

        DWORD options = 0;
        if (SUCCEEDED(dialog->GetOptions(&options)))
        {
            dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR);
        }

        if (SUCCEEDED(dialog->Show(hwnd)))
        {
            ComPtr<IShellItem> result;

            if (SUCCEEDED(dialog->GetResult(result.GetAddressOf())))
            {
                PWSTR path = nullptr;

                if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)))
                {
                    result_path = path;
                    CoTaskMemFree(path);
                }
            }
        }
    }

    return result_path;
}
} // namespace WinFilePicker
