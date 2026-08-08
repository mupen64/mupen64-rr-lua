/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/Config.hpp>
#include <Common.Views/IDialogService.hpp>
#include <components/Statusbar.hpp>

class WinDialogService : public IDialogService
{
  public:
    size_t show_multiple_choice_dialog(std::string_view id, const std::vector<std::wstring> &choices,
                                       const wchar_t *str, const wchar_t *title = nullptr,
                                       core_dialog_type type = fsvc_warning, void *hwnd = nullptr,
                                       const wchar_t *details = nullptr) override
    {
        const auto silenced = std::ranges::find(ALWAYS_LOUD_IDS, id) == ALWAYS_LOUD_IDS.end() && g_config.silent_mode;

        if (silenced)
        {
            const auto default_index = g_config.silent_mode_dialog_choices[IOUtils::to_wide_string(id)];
            g_view_logger->trace(L"[FrontendService] show_multiple_choice_dialog: '{}', silent mode answer: {}", str,
                                 default_index);
            return std::stoi(default_index);
        }

        auto result = dialog_choice_map.find(id);
        if (result != dialog_choice_map.end())
        {
            const auto answer = result->second;
            g_view_logger->trace(L"[FrontendService] show_multiple_choice_dialog: '{}', dont show again answer: {}",
                                 str, answer);
            return answer;
        }

        std::vector<TASKDIALOG_BUTTON> buttons;

        buttons.reserve(choices.size());
        for (int i = 0; i < choices.size(); ++i)
        {
            buttons.push_back({i, choices[i].c_str()});
        }

        auto icon = TD_ERROR_ICON;
        switch (type)
        {
        case fsvc_error:
            icon = TD_ERROR_ICON;
            break;
        case fsvc_warning:
            icon = TD_WARNING_ICON;
            break;
        case fsvc_information:
            icon = TD_INFORMATION_ICON;
            break;
        }

        TASKDIALOGCONFIG task_dialog_config = {
            .cbSize = sizeof(TASKDIALOGCONFIG),
            .hwndParent = static_cast<HWND>(hwnd ? hwnd : g_main_ctx.hwnd),
            .pszWindowTitle = title,
            .pszMainIcon = icon,
            .pszContent = str,
            .cButtons = (UINT)buttons.size(),
            .pButtons = buttons.data(),
            .pszVerificationText = L"Don't show again",
        };

        if (details)
        {
            task_dialog_config.dwFlags |= TDF_EXPAND_FOOTER_AREA;
            task_dialog_config.pszExpandedInformation = details;
            task_dialog_config.pszExpandedControlText = L"Show details";
            task_dialog_config.pszCollapsedControlText = L"Hide details";
        }

        int pressed_button = -1;
        BOOL dont_show_again = false;
        TaskDialogIndirect(&task_dialog_config, &pressed_button, nullptr, &dont_show_again);

        if (dont_show_again)
        {
            // directly construct key
            dialog_choice_map.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                                      std::forward_as_tuple(pressed_button));
        }

        g_view_logger->trace(
            L"[FrontendService] show_multiple_choice_dialog: '{}', manual answer: {}, dont show again: {}", str,
            pressed_button > 0 ? choices[pressed_button] : L"?", dont_show_again);

        return pressed_button;
    }

    bool show_ask_dialog(std::string_view id, const wchar_t *str, const wchar_t *title = nullptr, bool warning = false,
                         void *hwnd = nullptr) override
    {
        return show_multiple_choice_dialog(id, {L"Yes", L"No"}, str, title, warning ? fsvc_warning : fsvc_information,
                                           hwnd) == 0;
    }

    void show_dialog(const wchar_t *str, const wchar_t *title = nullptr, core_dialog_type type = fsvc_warning,
                     void *hwnd = nullptr) override
    {
        int icon = 0;

        switch (type)
        {
        case fsvc_error:
            g_view_logger->error(L"[FrontendService] {}", str);
            icon = MB_ICONERROR;
            break;
        case fsvc_warning:
            g_view_logger->warn(L"[FrontendService] {}", str);
            icon = MB_ICONWARNING;
            break;
        case fsvc_information:
            g_view_logger->info(L"[FrontendService] {}", str);
            icon = MB_ICONINFORMATION;
            break;
        default:
            assert(false);
        }

        if (!g_config.silent_mode)
        {
            MessageBox(static_cast<HWND>(hwnd ? hwnd : g_main_ctx.hwnd), str, title, icon);
        }
    }

    void show_statusbar(const wchar_t *str) override { Statusbar::post(str); }

  private:
    const std::vector<std::string> ALWAYS_LOUD_IDS = {VIEW_DLG_RAMSTART, VIEW_DLG_CONFIRM_SETTINGS_DISCARD};
    StrUtils::unordered_string_map<size_t> dialog_choice_map;
};

IDialogService *g_dialog_service = new WinDialogService();
