#ifndef SERVICE_QT_CORE_SERVICE_HPP_INCLUDED
#define SERVICE_QT_CORE_SERVICE_HPP_INCLUDED

#include "../view/MainWindow.hpp"
#include "../model/Core.hpp"

class QtCoreService final : Mupen::ICoreService
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
                                      std::string_view message, core_dialog_type type);

    /**
     * @brief Display an info dialog with an "OK" button.
     *
     * @param title The title of the dialog.
     * @param message The message of the dialog.
     * @param type The icon to display alongside the dialog text.
     */
    virtual void show_info_dialog(std::string_view title, std::string_view message, core_dialog_type type);

    /**
     * @brief Assuming the UI is switched into game view, requests that the render window be created and sized.
     *
     * @param settings The settings to apply.
     * @return mup_wm_handle A handle to the set-up window.
     */
    virtual mup_wm_handle setup_window(const mupv_wm_settings &settings);

  private:
    MainWindow *m_main_window;
    StrUtils::unordered_string_map<size_t> m_saved_choices;
};

#endif