/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "CoreUtils.h"
#include "Plugin.h"
#include "AppActions.h"

// Prompts the user to change their plugin selection.
static void prompt_plugin_change(HWND hwnd)
{
    auto result = DialogService::show_multiple_choice_dialog(
        VIEW_DLG_PLUGIN_LOAD_ERROR, {L"Choose Default Plugins", L"Change Plugins", L"Cancel"},
        L"One or more plugins couldn't be loaded.\r\nHow would you like to proceed?", L"Core", fsvc_error, hwnd);

    if (result == 0)
    {
        auto plugin_discovery_result = PluginUtil::discover_plugins(Config::plugin_directory());

        auto first_video_plugin = std::ranges::find_if(
            plugin_discovery_result.plugins, [](const auto &plugin) { return plugin->type() == plugin_video; });

        auto first_audio_plugin = std::ranges::find_if(
            plugin_discovery_result.plugins, [](const auto &plugin) { return plugin->type() == plugin_audio; });

        auto first_input_plugin = std::ranges::find_if(
            plugin_discovery_result.plugins, [](const auto &plugin) { return plugin->type() == plugin_input; });

        auto first_rsp_plugin = std::ranges::find_if(plugin_discovery_result.plugins,
                                                     [](const auto &plugin) { return plugin->type() == plugin_rsp; });

        if (first_video_plugin != plugin_discovery_result.plugins.end())
        {
            g_config.selected_video_plugin = first_video_plugin->get()->path();
        }

        if (first_audio_plugin != plugin_discovery_result.plugins.end())
        {
            g_config.selected_audio_plugin = first_audio_plugin->get()->path();
        }

        if (first_input_plugin != plugin_discovery_result.plugins.end())
        {
            g_config.selected_input_plugin = first_input_plugin->get()->path();
        }

        if (first_rsp_plugin != plugin_discovery_result.plugins.end())
        {
            g_config.selected_rsp_plugin = first_rsp_plugin->get()->path();
        }

        return;
    }

    if (result == 1)
    {
        g_config.settings_tab = 0;
        ActionManager::invoke(AppActions::SETTINGS);
    }
}

std::pair<std::string, std::string> CoreUtils::get_error_message_for_result(core_result result)
{
    if (result == Res_Ok || result == Res_Cancelled || result == VCR_InvalidControllers)
    {
        return {};
    }

    std::wstring module;
    std::wstring error;

    switch (result)
    {
#pragma region VCR
    case VCR_InvalidFormat:
        module = L"VCR";
        error = L"The provided data has an invalid format.";
        break;
    case VCR_BadFile:
        module = L"VCR";
        error = L"The provided file is inaccessible or does not exist.";
        break;
    case VCR_InvalidSavestate:
        module = L"VCR";
        error = L"The movie's savestate is missing or invalid.";
        break;
    case VCR_InvalidFrame:
        module = L"VCR";
        error = L"The resulting frame is outside the bounds of the movie.";
        break;
    case VCR_NoMatchingRom:
        module = L"VCR";
        error = L"There is no rom which matches this movie.";
        break;
    case VCR_Idle:
        module = L"VCR";
        error = L"The VCR engine is idle, but must be active to complete this operation.";
        break;
    case VCR_NotFromThisMovie:
        module = L"VCR";
        error = L"The provided freeze buffer is not from the currently active movie.";
        break;
    case VCR_InvalidVersion:
        module = L"VCR";
        error = L"The movie's version is invalid.";
        break;
    case VCR_InvalidExtendedVersion:
        module = L"VCR";
        error = L"The movie's extended version is invalid.";
        break;
    case VCR_NeedsPlaybackOrRecording:
        module = L"VCR";
        error = L"The operation requires a playback or recording task.";
        break;
    case VCR_NeedsPlayback:
        module = L"VCR";
        error = L"The operation requires a playback task.";
        break;
    case VCR_InvalidStartType:
        module = L"VCR";
        error = L"The provided start type is invalid.";
        break;
    case VCR_WarpModifyAlreadyRunning:
        module = L"VCR";
        error = L"Another warp modify operation is already running.";
        break;
    case VCR_WarpModifyNeedsRecordingTask:
        module = L"VCR";
        error = L"Warp modifications can only be performed during recording.";
        break;
    case VCR_WarpModifyEmptyInputBuffer:
        module = L"VCR";
        error = L"The provided input buffer is empty.";
        break;
    case VCR_SeekAlreadyRunning:
        module = L"VCR";
        error = L"Another seek operation is already running.";
        break;
    case VCR_SeekSavestateLoadFailed:
        module = L"VCR";
        error = L"The seek operation could not be initiated due to a savestate not being loaded successfully.";
        break;
    case VCR_SeekSavestateIntervalZero:
        module = L"VCR";
        error = L"The seek operation can't be initiated because the seek savestate interval is 0.";
        break;
#pragma endregion
#pragma region VR
    case VR_NoMatchingRom:
        module = L"Core";
        error = L"The ROM couldn't be loaded.\r\nCouldn't find an appropriate ROM.";
        break;
    case VR_PluginError:
        module = L"Core";
        error = L"One or more plugins couldn't be loaded.\r\nVerify that you have selected all four plugins.";
        break;
    case VR_RomInvalid:
        module = L"Core";
        error = L"The ROM couldn't be loaded.\r\nVerify that the ROM is a valid N64 ROM.";
        break;
    case VR_FileOpenFailed:
        module = L"Core";
        error = L"Failed to open streams to core files.\r\nVerify that Mupen is allowed disk access.";
        break;
#pragma endregion
#pragma region Init
    case IN_MissingComponent:
        module = L"Core";
        error = L"The core params are missing a critical component.";
        break;
#pragma endregion
    default:
        module = L"Unknown";
        error = L"Unknown error.";
        break;
    }
    return {std::move(module), std::move(error)};
}

bool CoreUtils::show_error_dialog_for_result(core_result result, HWND hwnd)
{
    g_view_logger->error("CoreUtils::show_error_dialog_for_result({}, {})", static_cast<int32_t>(result), hwnd);

    const auto [module, error] = get_error_message_for_result(result);
    if (error.empty()) return false;

    if (result == VR_PluginError)
    {
        prompt_plugin_change(hwnd);
        return true;
    }

    const auto title = std::format("{} Error {}", module, static_cast<int32_t>(result));
    DialogService::show_dialog(IOUtils::to_wide_string(error).c_str(), IOUtils::to_wide_string(title).c_str(),
                               fsvc_error, hwnd);

    return true;
}