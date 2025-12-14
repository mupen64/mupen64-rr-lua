/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "QtCoreService.hpp"
#include <QMessageBox>
#include <ranges>
#include <vector>
#include "StrUtils.h"
#include "../Utils.hpp"

/**
 * @brief Display a multiple-choice dialog, if the user has not requested to hide it.
 *
 * @param id An ID string.
 * @param choices The choices to select between.
 * @param title The title of the dialog.
 * @param message The message of the dialog.
 * @param type The icon to display alongside the dialog text.
 * @return Index of the chosen option; SIZE_MAX if cancelled. If the user has requested
 * not to see this dialog, returns the last chosen option.
 */
size_t QtCoreService::show_choice_dialog(std::string_view id, std::span<const std::string> choices,
                                         std::string_view title, std::string_view message, core_dialog_type type)
{
    {
        auto saved = m_saved_choices.find(id);
        if (saved != m_saved_choices.end())
        {
            // return the saved choice
            return saved->second;
        }
    }

    std::pair<size_t, bool> result;

    auto qt_choices = choices | std::views::transform(QString::fromStdString) | std::ranges::to<std::vector>();
    auto qt_title = str_to_qstring(title);
    auto qt_message = str_to_qstring(message);
    auto qt_icon = QMessageBox::NoIcon;
    switch (type)
    {
    case fsvc_error:
        qt_icon = QMessageBox::Critical;
        break;
    case fsvc_warning:
        qt_icon = QMessageBox::Warning;
        break;
    case fsvc_information:
        qt_icon = QMessageBox::Information;
        break;
    }

    bool call_worked = QMetaObject::invokeMethod(m_main_window, &MainWindow::showChoiceDialog, qReturnArg(result),
                                                 qt_choices, qt_title, qt_message, qt_icon);
    assert(call_worked);

    // save the choice if needed
    if (result.second)
    {
        m_saved_choices.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                                std::forward_as_tuple(result.first));
    }
    return result.first;
}

/**
 * @brief Display an info dialog with an "OK" button.
 *
 * @param title The title of the dialog.
 * @param message The message of the dialog.
 * @param type The icon to display alongside the dialog text.
 */
void QtCoreService::show_info_dialog(std::string_view title, std::string_view message, core_dialog_type type)
{
    auto qt_title = str_to_qstring(title);
    auto qt_message = str_to_qstring(message);
    auto qt_icon = QMessageBox::NoIcon;
    switch (type)
    {
    case fsvc_error:
        qt_icon = QMessageBox::Critical;
        break;
    case fsvc_warning:
        qt_icon = QMessageBox::Warning;
        break;
    case fsvc_information:
        qt_icon = QMessageBox::Information;
        break;
    }

    bool call_worked =
        QMetaObject::invokeMethod(m_main_window, &MainWindow::showInfoDialog, qt_title, qt_message, qt_icon);
    assert(call_worked);
}