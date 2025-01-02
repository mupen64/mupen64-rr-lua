#pragma once

/*
 *	Interface for calls originating from core or shared layer to the view
 *layer
 *
 *	Must be implemented in the view layer.
 */

#include <functional>
#include <shared/Config.hpp>
#include <shared/types/CoreTypes.h>

namespace FrontendService {
/**
 * The tone of a dialog.
 */
enum class DialogType {
  Error,
  Warning,
  Information,
};

/**
 * Prompts the user to pick a choice from a provided collection of choices.
 * \param choices The collection of choices.
 * \param str The dialog content.
 * \param title The dialog title.
 * \param hwnd The parent window
 * \return The index of the chosen choice
 */
size_t show_multiple_choice_dialog(const std::vector<std::wstring> &choices,
                                   const wchar_t *str,
                                   const wchar_t *title = nullptr,
                                   DialogType type = DialogType::Warning,
                                   void *hwnd = nullptr);

/**
 * \brief Asks the user a yes/no question
 * \param str The dialog content
 * \param title The dialog title
 * \param warning Whether the tone of the message is perceived as a warning
 * \param hwnd The parent window
 * \return Whether the user answered yes
 * \remarks If the user has chosen to not use modals, this function will return
 * true by default
 */
bool show_ask_dialog(const wchar_t *str, const wchar_t *title = nullptr,
                     bool warning = false, void *hwnd = nullptr);

/**
 * \brief Shows the user a dialog.
 * \param str The dialog content.
 * \param title The dialog title.
 * \param type The dialog tone.
 * \param hwnd The parent window.
 */
void show_dialog(const wchar_t *str, const wchar_t *title = nullptr,
                 DialogType type = DialogType::Warning, void *hwnd = nullptr);

/**
 * \brief Shows text in the statusbar
 * \param str The text
 */
void show_statusbar(const wchar_t *str);

/**
 * \brief Gets the path to the directory containing the mupen64 executable
 */
std::filesystem::path get_app_path();

/**
 * \brief Fills out the default hotkey keys in the config structure
 * \param config The config structure to modify
 * \remarks Because the shared layer is unaware of keycodes, the view must do
 * this.
 * FIXME: While this works for now, storing platform-dependent keycodes in the
 * config will make config files incompatible across platforms.
 */
void set_default_hotkey_keys(t_config *config);

/**
 * \brief Gets the native handle of the app instance
 */
void *get_app_instance_handle();

/**
 * \brief Gets the native handle of the main emulator window
 */
void *get_main_window_handle();

/**
 * \brief Gets the native handle of the statusbar. Can be null.
 */
void *get_statusbar_handle();

/**
 * \brief Gets the native handle of the parent window of the plugin
 * configuration dialog
 */
void *get_plugin_config_parent_handle();

/**
 * \brief Gets whether the view prefers the core to not skip rendering any
 * frames
 */
bool get_prefers_no_render_skip();

/**
 * \brief Updates the screen with the current visual information
 */
void update_screen();

/**
 * \brief Notifies the frontend of a new VI
 */
void at_vi();

/**
 * \brief Notifies the frontend of the audio changing
 */
void ai_len_changed();

/**
 * \brief Finds the first rom from the available ROM list which matches the
 * predicate \param predicate A predicate which determines if the rom matches
 * \return The rom's path, or an empty string if no rom was found
 */
std::wstring
find_available_rom(const std::function<bool(const t_rom_header &)> &predicate);

/**
 * Gets the current video size from the MGE compositor
 * \param width The video width
 * \param height The video height
 */
void mge_get_video_size(int32_t *width, int32_t *height);

/**
 * \brief Writes the MGE compositor's current emulation front buffer into the
 * destination buffer. \param buffer The video buffer. Must be at least of size
 * <c>width * height * 3</c>, as acquired by <c>mge_get_video_size</c>.
 */
void mge_copy_video(void *buffer);

/**
 * \brief Draws the given data to the MGE surface
 * \param data The buffer holding video data. Must be at least of size <c>width
 * * height * 3</c>, as acquired by <c>mge_get_video_size</c>. \remarks The
 * video buffer's size must match the current video size provided by
 * <c>get_video_size</c>.
 */
void mge_load_screen(void *data);
} // namespace FrontendService
