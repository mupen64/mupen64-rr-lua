#include "QtCoreService.hpp"
#include <QMessageBox>
#include "StrUtils.h"
#include "../utils.hpp"

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
                                         std::string_view title, std::string_view message, core_dialog_type type) const
{
    {
        auto saved = m_saved_choices.find(id);
        if (saved != m_saved_choices.end())
        {
            // return the saved choice
            return saved->second;
        }
    }
    // QMetaObject::invokeMethod()
}

/**
 * @brief Display an info dialog with an "OK" button.
 *
 * @param title The title of the dialog.
 * @param message The message of the dialog.
 * @param type The icon to display alongside the dialog text.
 */
void QtCoreService::show_info_dialog(std::string_view title, std::string_view message, core_dialog_type type) const
{
}

/**
 * @brief Assuming the UI is switched into game view, requests that the render window be created and sized.
 *
 * @param settings The settings to apply.
 * @return mup_wm_handle A handle to the set-up window.
 */
mup_wm_handle QtCoreService::setup_window(const mupv_wm_settings &settings) const
{
}