/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <m64rr/Types.hpp>

/**
 * \brief Provides dialog and notification-related functionality.
 */
class IDialogService
{
  public:
    virtual ~IDialogService() = default;

    /**
     * \brief Prompts the user to select from a provided collection of choices.
     * \param id The dialog's unique identifier. Used for correlating a user's choice with a dialog.
     * \param choices The collection of choices.
     * \param str The dialog content.
     * \param title The dialog title.
     * \param hwnd The parent window. If nullptr, the main window will be used.
     * \param details The details section. If std::nullopt, no details section will be shown.
     * \return The index of the chosen choice. If the user has chosen to not use modals, this function will return the
     * index specified by the user's preferences in the view. If the user has chosen to not show the dialog again, this
     * function will return the last choice.
     */
    virtual size_t show_multiple_choice_dialog(std::string_view id, const std::vector<std::wstring> &choices,
                                               std::string_view str, std::optional<std::string_view> title = std::nullopt,
                                               core_dialog_type type = fsvc_warning, void *hwnd = nullptr,
                                               std::optional<std::string_view> details = std::nullopt) = 0;

    /**
     * \brief Asks the user a Yes/No question.
     * \param id The dialog's unique identifier. Used for correlating a user's choice with a dialog.
     * \param str The dialog content.
     * \param title The dialog title.
     * \param warning Whether the tone of the message is perceived as a warning.
     * \param hwnd The parent window. If nullptr, the main window will be used.
     * \return Whether the user answered Yes. If the user has chosen to not use modals, this function will return the
     * value specified by the user's preferences in the view. If the user has chosen to not show the dialog again, this
     * function will return the last choice.
     */
    virtual bool show_ask_dialog(std::string_view id, std::string_view str, std::optional<std::string_view> title = std::nullopt,
                                 bool warning = false, void *hwnd = nullptr) = 0;

    /**
     * \brief Shows the user a dialog with the specified content.
     * \param str The dialog content.
     * \param title The dialog title.
     * \param type The dialog tone.
     * \param hwnd The parent window. If nullptr, the main window will be used.
     */
    virtual void show_dialog(std::string_view str, std::optional<std::string_view> title = std::nullopt, core_dialog_type type = fsvc_warning,
                             void *hwnd = nullptr) = 0;

    /**
     * \brief Shows text in the statusbar.
     * \param str The text.
     */
    virtual void show_statusbar(std::string_view str) = 0;
};
