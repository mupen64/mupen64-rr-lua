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

/**
 * @brief Command-line implementation of the dialog service.
 * 
 * Serves as a reasonable default until the GUI overrides it.
 */
namespace DialogService
{
void print_header(std::string_view title, core_dialog_type type)
{
    using namespace std::literals;
    std::string_view type_str;
    switch (type)
    {
    case fsvc_error:
        type_str = "ERROR"sv;
        break;
    case fsvc_warning:
        type_str = "WARN"sv;
        break;
    case fsvc_information:
        type_str = "INFO"sv;
        break;
    }

    std::println("===============================");
    std::println("{} [{}]", title, type_str);
    std::println("-------------------------------");
}

size_t show_multiple_choice_dialog(std::string_view id, const std::vector<std::string> &choices, std::string_view str,
                                   std::optional<std::string_view> title, core_dialog_type type, void *hwnd,
                                   std::optional<std::string_view> details)
{
    print_header(title.value_or(""), type);

    std::string input_line;
    while (true)
    {
        std::println("{}", str);
        for (size_t i = 0; i < choices.size(); i++)
        {
            std::println("{}) {}", (i + 1), choices[i]);
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

bool show_ask_dialog(std::string_view id, std::string_view str, std::optional<std::string_view> title, bool warning,
                     void *hwnd)
{
    print_header(title.value_or(""), warning ? fsvc_warning : fsvc_information);

    std::string input_line;
    while (true)
    {
        std::println("{}", str);
        std::println("(enter Y for yes, N for no)");
        std::print("> ");

        std::getline(std::cin, input_line);
        if (input_line.size() == 1)
        {
            if (input_line[0] == 'Y' || input_line[0] == 'y') return true;
            if (input_line[0] == 'N' || input_line[0] == 'n') return false;
        }
        std::println("Invalid input...");
    }
}

void show_dialog(std::string_view str, std::optional<std::string_view> title, core_dialog_type type, void *hwnd)
{
    print_header(title.value_or(""), type);

    std::println("{}", str);
    std::print("(press [Enter] to continue)");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), std::cin.widen('\n'));
}

void show_statusbar(std::string_view str)
{
    std::println("{}", str);
}
} // namespace DialogService
