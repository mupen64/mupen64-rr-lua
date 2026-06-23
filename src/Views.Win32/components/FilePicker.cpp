/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdafx.h>
#include <Config.hpp>
#include <components/FilePicker.hpp>
#include <WinFilePicker.hpp>

std::filesystem::path FilePicker::show_open_dialog(const std::wstring &id, HWND hwnd, const std::wstring &filter)
{
    RT_ASSERT(is_on_gui_thread(), L"FilePicker::show_open_dialog called from non-GUI thread");

    std::filesystem::path restored_path =
        g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    g_view_logger->info(L"file dialog {}: {}\n", id, restored_path.wstring());

    const auto result = WinFilePicker::show_open_dialog(hwnd, IOUtils::to_utf8_string(filter), restored_path);

    if (!result.empty())
    {
        g_config.persistent_folder_paths[id] = result;
        return g_config.persistent_folder_paths[id];
    }

    return {};
}

std::filesystem::path FilePicker::show_save_dialog(const std::wstring &id, HWND hwnd, const std::wstring &filter)
{
    RT_ASSERT(is_on_gui_thread(), L"FilePicker::show_save_dialog called from non-GUI thread");

    std::filesystem::path restored_path =
        g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    g_view_logger->info(L"file dialog {}: {}\n", id, restored_path.wstring());

    const auto result = WinFilePicker::show_save_dialog(hwnd, IOUtils::to_utf8_string(filter), restored_path);

    if (!result.empty())
    {
        g_config.persistent_folder_paths[id] = result;
        return g_config.persistent_folder_paths[id];
    }

    return {};
}

std::filesystem::path FilePicker::show_folder_dialog(const std::wstring &id, HWND hwnd)
{
    RT_ASSERT(is_on_gui_thread(), L"FilePicker::show_folder_dialog called from non-GUI thread");

    std::filesystem::path restored_path =
        g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    std::wstring final_path;
    IFileDialog *pfd;

    g_view_logger->info(L"folder dialog {}: {}\n", id, restored_path.wstring());

    const auto result = WinFilePicker::show_folder_dialog(hwnd, restored_path);

    if (!result.empty())
    {
        g_config.persistent_folder_paths[id] = result;
        return g_config.persistent_folder_paths[id];
    }

    return {};
}
