#pragma once

#include <Windows.h>
#include <string>

/**
 * \brief Shows a file selection dialog
 * \param id The dialog's unique identifier, which is used for persisting the
 * path \param hwnd The dialog owner's window handle, or NULL \param filter A
 * collection of valid extensions in the format *.ext separated by semicolons
 * \return The chosen file's path
 */
std::wstring show_persistent_open_dialog(const std::wstring &id, HWND hwnd,
                                         const std::wstring &filter);

/**
 * \brief Shows a file save dialog
 * \param id The dialog's unique identifier, which is used for persisting the
 * path \param hwnd The dialog owner's window handle, or NULL \param filter A
 * collection of valid extensions in the format *.ext separated by semicolons
 * \return The chosen file's path
 */
std::wstring show_persistent_save_dialog(const std::wstring &id, HWND hwnd,
                                         const std::wstring &filter);

/**
 * \brief Shows a folder selection dialog
 * \param id The dialog's unique identifier, which is used for persisting the
 * path \param hwnd The dialog owner's window handle, or NULL \return The chosen
 * folder's path
 */
std::wstring show_persistent_folder_dialog(const std::wstring &id, HWND hwnd);
