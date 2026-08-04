/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Config.hpp>
#include <DialogService.hpp>
#include <winhttp.h>
#include <components/TextEditDialog.hpp>
#include <components/UpdateChecker.hpp>
#include <nlohmann/json.hpp>

namespace
{
constexpr std::wstring get_channel()
{
    const std::string release_type =
#ifdef NIGHTLY
        "nightly";
#else
        "stable";
#endif
    const std::string arch = sizeof(void *) == 8 ? "w64" : "w32";
    return IOUtils::to_wide_string(release_type + "-" + arch);
}

bool launch_updater(const std::wstring &channel)
{
    const auto script_path = IOUtils::exe_path().parent_path() / L"update.py";

    if (!std::filesystem::exists(script_path))
    {
        g_view_logger->error(L"[UpdateChecker] update.py not found next to the executable.");
        return false;
    }

    struct PythonLauncher
    {
        const wchar_t *exe;
        const wchar_t *args;
    };

    // `py` is the standard Windows Python launcher; fall back to the plain
    // interpreter names for installations that don't register it.
    constexpr PythonLauncher launchers[] = {
        {L"py", L"-3"},
        {L"python", L""},
        {L"python3", L""},
    };

    for (const auto &launcher : launchers)
    {
        std::wstring command_line =
            std::format(L"{} {} \"{}\" --channel {}", launcher.exe, launcher.args, script_path.wstring(), channel);

        STARTUPINFO si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        // CREATE_NEW_CONSOLE gives the TUI its own window and keeps it alive
        // independently of mupen.
        if (CreateProcess(nullptr, command_line.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr,
                          IOUtils::exe_path().parent_path().c_str(), &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            g_view_logger->info(L"[UpdateChecker] Launched auto-updater with channel {}.", channel);
            return true;
        }

        g_view_logger->trace(L"[UpdateChecker] Failed to launch '{}' ({}).", launcher.exe, GetLastError());
    }

    g_view_logger->error(L"[UpdateChecker] No Python interpreter found; update cancelled.");
    return false;
}
} // namespace

namespace UpdateChecker
{
const std::wstring REPO_LATEST_RELEASE_URL = L"/repos/mupen64/mupen64-rr-lua/releases/latest";

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

    HINTERNET h_request = WinHttpOpenRequest(h_connect, L"GET", REPO_LATEST_RELEASE_URL.c_str(), NULL,
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
int version_compare(const std::wstring &version1, const std::wstring &version2)
{
    auto split_version = [](const std::wstring &version) {
        std::vector parts(4, 0);
        const std::size_t dash_pos = version.find(L'-');
        std::wstring main_part = dash_pos != std::wstring::npos ? version.substr(0, dash_pos) : version;
        const std::wstring sub_part = dash_pos != std::wstring::npos ? version.substr(dash_pos + 1) : L"";

        std::wstringstream ss(main_part);
        for (int i = 0; i < 3 && std::getline(ss, main_part, L'.'); ++i)
        {
            try
            {
                parts[i] = std::stoi(main_part);
            }
            catch (...)
            {
                parts[i] = 0;
            }
        }

        if (!sub_part.empty())
        {
            try
            {
                parts[3] = std::stoi(sub_part);
            }
            catch (...)
            {
                parts[3] = 0;
            }
        }

        return parts;
    };

    const std::vector<int> parts1 = split_version(version1);
    const std::vector<int> parts2 = split_version(version2);

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
        DialogService::show_dialog(L"Failed to fetch update information. Please try again later.", L"Update Error",
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

    auto version = IOUtils::to_wide_string(tag_name.get<std::string>());

    if (!manual && g_config.ignored_version == version)
    {
        g_view_logger->trace(L"[UpdateChecker] Version {} ignored by user. Skipping showing update dialog.", version);
        return;
    }

    const auto version_difference = version_compare(CURRENT_VERSION, version);

    if (version_difference != -1)
    {
        if (manual)
        {
            DialogService::show_dialog(L"You are already up-to-date.", L"Already up-to-date", fsvc_information);
        }

        return;
    }

show_prompt:

    const auto result = DialogService::show_multiple_choice_dialog(
        VIEW_DLG_UPDATE_DIALOG,
        {
            L"Update Now",
            L"Show Changelog",
            L"Skip Version",
        },
        std::format(L"Mupen64 {} is available for download.", version).c_str(), L"Update Available", fsvc_information);

    switch (result)
    {
    case 0: {
        if (!launch_updater(get_channel()))
        {
            DialogService::show_dialog(L"Failed to launch the auto-updater.\n\n"
                                       L"Make sure Python 3 is installed and available, then run update.py "
                                       L"from the mupen directory.",
                                       L"Update Error", fsvc_error);
        }
        break;
    }
    case 1: {
        const auto changelog = IOUtils::to_wide_string(body.get<std::string>());
        TextEditDialog::show(
            {.parent_hwnd = g_main_ctx.hwnd, .text = changelog, .caption = L"Changelog", .readonly = true});
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
