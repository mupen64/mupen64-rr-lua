/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/Config.hpp>
#include <Common.Views/IDialogService.hpp>
#include <winhttp.h>
#include <components/TextEditDialog.hpp>
#include <components/UpdateChecker.hpp>
#include <nlohmann/json.hpp>

namespace UpdateChecker
{
const std::string REPO_LATEST_RELEASE_URL = "/repos/mupen64/mupen64-rr-lua/releases/latest";

/**
 * Gets information about the latest release using the Github REST API.
 */
std::string get_latest_release_as_json()
{
    HINTERNET h_session =
        WinHttpOpen(L"Win32 Mupen64 GitHub API Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);

    if (!h_session)
    {
        g_view_logger->error("[UpdateChecker] WinHttpOpen failed");
        return "";
    }

    DWORD secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    if (!WinHttpSetOption(h_session, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols)))
    {
        g_view_logger->error("[UpdateChecker] Failed to set TLS 1.2 protocol");
        return "";
    }

    HINTERNET h_connect = WinHttpConnect(h_session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!h_connect)
    {
        g_view_logger->error("[UpdateChecker] WinHttpConnect failed");
        WinHttpCloseHandle(h_session);
        return "";
    }

    HINTERNET h_request =
        WinHttpOpenRequest(h_connect, L"GET", IOUtils::to_wide_string(REPO_LATEST_RELEASE_URL).c_str(), NULL,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    if (!h_request)
    {
        g_view_logger->error("[UpdateChecker] WinHttpOpenRequest failed");
        WinHttpCloseHandle(h_connect);
        WinHttpCloseHandle(h_session);
        return "";
    }

    BOOL b_results = WinHttpSendRequest(h_request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (!b_results)
    {
        g_view_logger->error("[UpdateChecker] WinHttpSendRequest failed");
        WinHttpCloseHandle(h_request);
        WinHttpCloseHandle(h_connect);
        WinHttpCloseHandle(h_session);
        return "";
    }

    b_results = WinHttpReceiveResponse(h_request, NULL);

    if (!b_results)
    {
        g_view_logger->error("[UpdateChecker] WinHttpSendRequest failed");
        WinHttpCloseHandle(h_request);
        WinHttpCloseHandle(h_connect);
        WinHttpCloseHandle(h_session);
        return "";
    }

    std::stringstream response_stream;
    DWORD size = 0;
    DWORD downloaded = 0;
    char buf[4096]{};
    do
    {
        size = 0;
        if (WinHttpQueryDataAvailable(h_request, &size) && size > 0 &&
            WinHttpReadData(h_request, buf, size, &downloaded))
        {
            response_stream.write(buf, downloaded);
        }
    } while (size > 0);

    WinHttpCloseHandle(h_request);
    WinHttpCloseHandle(h_connect);
    WinHttpCloseHandle(h_session);

    return response_stream.str();
}

/**
 * Compares two version strings.
 *
 * Returns:
 * 1 if LHS > RHS
 * 0 if LHS = RHS
 * -1 if LHS < RHS
 */
int version_compare(std::string_view version1, std::string_view version2)
{
    // Splits a version in the form "X.Y.Z-W" into its component parts.
    const auto split_version = [](std::string_view version) {
        std::array<int, 4> parts{0};
        const std::size_t dash_pos = version.find(L'-');
        std::string_view main_part = dash_pos != std::string_view::npos ? version.substr(0, dash_pos) : version;
        std::string_view sub_part = dash_pos != std::string_view::npos ? version.substr(dash_pos + 1) : "";

        for (auto [i, part] : StrUtils::split_string(main_part, ".") | std::views::take(3) | std::views::enumerate)
        {
            auto [end_ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), parts[i]);
            if (ec != std::errc{}) parts[i] = 0;
        }

        if (!sub_part.empty())
        {
            auto [end_ptr, ec] = std::from_chars(sub_part.data(), sub_part.data() + sub_part.size(), parts[3]);
            if (ec != std::errc{}) parts[3] = 0;
        }

        return parts;
    };

    const auto parts1 = split_version(version1);
    const auto parts2 = split_version(version2);

    for (int i = 0; i < 4; ++i)
    {
        if (parts1[i] > parts2[i])
        {
            return 1;
        }
        if (parts1[i] < parts2[i])
        {
            return -1;
        }
    }
    return 0;
}

void show_connectivity_error(bool manual)
{
    if (manual)
    {
        DialogService::show_notification("Failed to fetch update information. Please try again later.", "Update Error",
                                         fsvc_error);
    }
}

void check(bool manual)
{
    if (!manual && !g_config.automatic_update_checking)
    {
        g_view_logger->trace("[UpdateChecker] Automatic update checking disabled. Ignoring update check.");
        return;
    }

    const auto json = get_latest_release_as_json();

    if (json.empty())
    {
        show_connectivity_error(manual);
        return;
    }

    nlohmann::json data = nlohmann::json::parse(json);

    const auto tag_name = data["tag_name"];

    if (!tag_name.is_string())
    {
        g_view_logger->error("[UpdateChecker] no tag_name in json response");
        show_connectivity_error(manual);
        return;
    }

    const auto body = data["body"];

    if (!body.is_string())
    {
        g_view_logger->error("[UpdateChecker] no body in json response");
        show_connectivity_error(manual);
        return;
    }

    auto version = tag_name.get<std::string>();

    if (!manual && g_config.ignored_version == version)
    {
        g_view_logger->trace("[UpdateChecker] Version {} ignored by user. Skipping showing update dialog.", version);
        return;
    }

    const auto version_difference = version_compare(CURRENT_VERSION, version);

    if (version_difference >= 0)
    {
        if (manual)
        {
            DialogService::show_notification("You are already up-to-date.", "Already up-to-date", fsvc_information);
        }

        return;
    }

show_prompt:

    const auto result = DialogService::show_multiple_choice_dialog(
        VIEW_DLG_UPDATE_DIALOG, {"Update Now", "Show Changelog", "Skip Version", "Remind Me Later"},
        std::format("Mupen64 {} is available for download.", version), "Update Available", fsvc_information);

    switch (result)
    {
    case 0:
        ShellExecute(0, 0, "https://mupen64.com", 0, 0, SW_SHOW);
        PostMessage(g_main_ctx.hwnd, WM_CLOSE, 0, 0);
        break;
    case 1: {
        const auto changelog = body.get<std::string>();
        TextEditDialog::show(
            {.parent_hwnd = g_main_ctx.hwnd, .text = changelog, .caption = "Changelog", .readonly = true});
        goto show_prompt;
    }
    case 2:
        g_config.ignored_version = version;
        break;
    default:
        break;
    }
}
} // namespace UpdateChecker
