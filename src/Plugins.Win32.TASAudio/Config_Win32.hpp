#pragma once

#include "Config.hpp"

namespace SDLAudio
{
// Displays a Win32 dialog box for the provided config object.
bool win32_show_config(HWND parent, Config &config);
} // namespace SDLAudio