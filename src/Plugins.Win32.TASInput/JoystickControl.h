/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing a joystick control.
 */
namespace JoystickControl
{
    const auto CLASS_NAME = L"JOYSTICK";
    constexpr auto WM_JOYSTICK_POSITION_CHANGED = WM_USER + 1;
    constexpr auto WM_JOYSTICK_DRAG_BEGIN = WM_USER + 2;

    void register_class(HINSTANCE hinst);

    /**
     * \brief Gets the joystick's position.
     * \param hwnd Handle to a joystick control.
     * \param x The x coordinate of the joystick.
     * \param y The y coordinate of the joystick.
     * \return Whether the operation was successful.
     */
    BOOL get_position(HWND hwnd, int* x, int* y);

    /**
     * \brief Sets the joystick's position.
     * \param hwnd Handle to a joystick control.
     * \param x The x coordinate of the joystick.
     * \param y The y coordinate of the joystick.
     * \return Whether the operation was successful.
     */
    BOOL set_position(HWND hwnd, int x, int y);
} // namespace JoystickControl
