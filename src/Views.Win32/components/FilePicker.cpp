/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.hpp>
#include <Common.Views/Config.hpp>
#include <components/FilePicker.hpp>
#include <WinFilePicker.hpp>
#include <Assert.hpp>

std::filesystem::path FilePicker::show_open_dialog(const std::string &id, HWND hwnd, const std::string &filter)
{
    NEED(is_on_gui_thread(), "FilePicker::show_open_dialog called from non-GUI thread");

    std::filesystem::path restored_path =
        g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    g_view_logger->info("file dialog {}: {}\n", id, restored_path.string());

    const auto result = WinFilePicker::show_open_dialog(hwnd, filter, restored_path);

    if (!result.empty())
    {
        g_config.persistent_folder_paths[id] = result.string();
        return g_config.persistent_folder_paths[id];
    }

    return {};
}

std::filesystem::path FilePicker::show_save_dialog(const std::string &id, HWND hwnd, const std::string &filter)
{
    NEED(is_on_gui_thread(), "FilePicker::show_save_dialog called from non-GUI thread");

    std::filesystem::path restored_path =
        g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    g_view_logger->info("file dialog {}: {}\n", id, restored_path.string());

    const auto result = WinFilePicker::show_save_dialog(hwnd, filter, restored_path);

    if (!result.empty())
    {
        g_config.persistent_folder_paths[id] = result.string();
        return g_config.persistent_folder_paths[id];
    }

    return {};
}

std::filesystem::path FilePicker::show_folder_dialog(const std::string &id, HWND hwnd)
{
    NEED(is_on_gui_thread(), "FilePicker::show_folder_dialog called from non-GUI thread");

    std::filesystem::path restored_path =
        g_config.persistent_folder_paths.contains(id) ? g_config.persistent_folder_paths[id] : get_desktop_path();

    std::string final_path;
    IFileDialog *pfd;

    g_view_logger->info("folder dialog {}: {}\n", id, restored_path.string());

    const auto result = WinFilePicker::show_folder_dialog(hwnd, restored_path);

    if (!result.empty())
    {
        g_config.persistent_folder_paths[id] = result.string();
        return g_config.persistent_folder_paths[id];
    }

    return {};
}
