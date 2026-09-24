/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/Assert.hpp>

namespace
{
[[noreturn]] void assertion_failed(std::string_view message)
{
    const std::string display_message = std::format("An error has occured:\n\n{}\n\nThe application will now exit.",
        message.empty() ? "No further information." : std::string(message));
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Assertion failed", display_message.c_str(), nullptr);

    std::abort();
}
} // namespace

void need(bool condition, std::string_view message)
{
    if (!condition) assertion_failed(message);
}

#ifdef _WIN32
void need(int condition, std::string_view message)
{
    if (condition == 0) assertion_failed(message);
}

void need(HRESULT hr, std::string_view message)
{
    if (SUCCEEDED(hr)) return;

    const auto hresult_message = std::format("{} (HRESULT 0x{:08X})", message, static_cast<unsigned long>(hr));
    assertion_failed(hresult_message);
}
#endif
