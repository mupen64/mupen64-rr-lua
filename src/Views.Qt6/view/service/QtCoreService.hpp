/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SERVICE_QT_CORE_SERVICE_HPP_INCLUDED
#define SERVICE_QT_CORE_SERVICE_HPP_INCLUDED

#include "../view/MainWindow.hpp"
#include "../model/Core.hpp"

class QtCoreService final : public Mupen::ICoreService
{
  public:
    QtCoreService(MainWindow *main_window) : Mupen::ICoreService(), m_main_window(main_window), m_saved_choices() {}

    virtual ~QtCoreService() {}

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
    virtual size_t show_choice_dialog(std::string_view id, std::span<const std::string> choices, std::string_view title,
                                      std::string_view message, core_dialog_type type) override;

    /**
     * @brief Display an info dialog with an "OK" button.
     *
     * @param title The title of the dialog.
     * @param message The message of the dialog.
     * @param type The icon to display alongside the dialog text.
     */
    virtual void show_info_dialog(std::string_view title, std::string_view message, core_dialog_type type) override;

    /**
     * @brief Creates an IWindowService for the specified graphics API.
     * 
     * @param api 
     * @return std::unique_ptr<Mupen::IWindowService> 
     */
    virtual std::unique_ptr<Mupen::IWindowService> init_window_service(mupv_graphics_api api) override;

  private:
    MainWindow *m_main_window;
    StrUtils::unordered_string_map<size_t> m_saved_choices;
};

#endif