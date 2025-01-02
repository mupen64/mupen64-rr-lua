#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>

namespace Statusbar
{
    enum class Section {
        Notification,
        VCR,
        Readonly,
        Input,
        Rerecords,
        FPS,
        VIs,
        Slot,
    };

    /**
     * \brief Gets the statusbar handle
     */
    HWND hwnd();

    /**
     * \brief Initializes the statusbar
     */
    void init();

    /**
     * \brief Shows text in the statusbar
     * \param text The text to be displayed
     * \param section The statusbar section to display the text in
     */
    void post(const std::wstring& text, Section section = Section::Notification);
} // namespace Statusbar
