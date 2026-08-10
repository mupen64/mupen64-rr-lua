/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/IDialogService.hpp>
#include <IOUtils.hpp>
#include <charconv>
#include <iostream>
#include <print>

class DialogService : public IDialogService
{
  public:
    size_t show_multiple_choice_dialog(std::string_view id, const std::vector<std::wstring> &choices,
                                       const wchar_t *str, const wchar_t *title = nullptr,
                                       core_dialog_type type = fsvc_warning, void *hwnd = nullptr,
                                       const wchar_t *details = nullptr) override
    {
        const auto ustr = IOUtils::to_utf8_string(str);
        const auto utitle = IOUtils::to_utf8_string(title);
        std::vector<std::string> uchoices;
        for (const auto &choice : choices) uchoices.push_back(IOUtils::to_utf8_string(choice));

        print_header(utitle, type);

        std::string input_line;
        while (true)
        {
            std::println("{}", ustr);
            for (size_t i = 0; i < choices.size(); i++)
            {
                std::println("{}) {}", (i + 1), uchoices[i]);
            }
            std::println("(enter an index, or press [Return] to cancel)");
            std::print("> ");
            std::getline(std::cin, input_line);
            if (input_line.empty()) return -1;

            int index = 0;
            auto [ptr, ec] = std::from_chars(input_line.data(), input_line.data() + input_line.size(), index);
            if (ec == std::errc{} && 0 < index && index <= choices.size())
            {
                return index - 1;
            }
            std::println("Invalid input...");
        }
    }

    bool show_ask_dialog(std::string_view id, const wchar_t *str, const wchar_t *title = nullptr, bool warning = false,
                         void *hwnd = nullptr) override
    {
        const auto ustr = IOUtils::to_utf8_string(str);
        const auto utitle = IOUtils::to_utf8_string(title);

        print_header(utitle, warning ? fsvc_warning : fsvc_information);

        std::string input_line;
        while (true)
        {
            std::println("{}", ustr);
            std::println("(enter Y for yes, N for no)");
            std::print("> ");

            std::getline(std::cin, input_line);
            if (input_line.size() == 1)
            {
                if (input_line[0] == 'Y' || input_line[0] == 'y') return true;
                if (input_line[0] == 'N' || input_line[0] == 'n') return true;
            }
            std::println("Invalid input...");
        }
    }

    void show_dialog(const wchar_t *str, const wchar_t *title = nullptr, core_dialog_type type = fsvc_warning,
                     void *hwnd = nullptr) override
    {
        const auto ustr = IOUtils::to_utf8_string(str);
        const auto utitle = IOUtils::to_utf8_string(title);

        print_header(utitle, type);

        std::println("{}", ustr);
        std::print("(press [Enter] to continue)");
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), std::cin.widen('\n'));
    }

    void show_statusbar(const wchar_t *str) override
    {
        const auto ustr = IOUtils::to_utf8_string(str);

        std::println("{}", ustr);
    }

  private:
    void print_header(std::string_view title, core_dialog_type type)
    {
        using namespace std::literals;
        std::string_view type_str;
        switch (type)
        {
        case fsvc_error:
            type_str = "ERROR"sv;
        case fsvc_warning:
            type_str = "WARN"sv;
        case fsvc_information:
            type_str = "INFO"sv;
            break;
        }

        std::println("===============================");
        std::println("{} [{}]", title, type_str);
        std::println("-------------------------------");
    }
};

IDialogService *g_dialog_service = new DialogService();
