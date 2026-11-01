/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "GamepadManager.hpp"
#include <algorithm>
#include <cmath>

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>

namespace
{
constexpr int POLL_INTERVAL_MS = 8;
constexpr int SCAN_INTERVAL_TICKS = 60;
constexpr int16_t AXIS_THRESHOLD = 16000;
constexpr int STICK_DEADZONE = 8;

int remapAxis(int16_t value)
{
    const float v = static_cast<float>(value) / 32767.0f;
    return std::clamp(static_cast<int>(std::lround(v * 128.0f)), -128, 127);
}
}

TASGamepadManager::TASGamepadManager(QObject *parent) : QObject(parent)
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    m_sdl_ready = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (!m_sdl_ready)
    {
        qWarning() << "TASInput: failed to init SDL gamepad subsystem:" << SDL_GetError();
        return;
    }
    SDL_SetJoystickEventsEnabled(false);
    SDL_SetGamepadEventsEnabled(false);

    openGamepad();

    m_timer.setInterval(POLL_INTERVAL_MS);
    QObject::connect(&m_timer, &QTimer::timeout, this, &TASGamepadManager::poll);
    m_timer.start();
}

TASGamepadManager::~TASGamepadManager()
{
    m_timer.stop();
    if (!m_sdl_ready) return;
    if (m_gamepad) SDL_CloseGamepad(m_gamepad);
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void TASGamepadManager::setEnabled(bool value)
{
    if (value == m_enabled) return;
    m_enabled = value;
    enabledChanged();
}

void TASGamepadManager::openGamepad()
{
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids && count > 0)
    {
        m_gamepad = SDL_OpenGamepad(ids[0]);
        if (m_gamepad)
        {
            const char *name = SDL_GetGamepadName(m_gamepad);
            m_device_name = name ? QString::fromUtf8(name) : QString();
            qInfo() << "TASInput: using gamepad" << m_device_name;
            connectedChanged();
        }
    }
    SDL_free(ids);
}

void TASGamepadManager::closeGamepad()
{
    SDL_CloseGamepad(m_gamepad);
    m_gamepad = nullptr;
    m_device_name.clear();
    connectedChanged();
}

void TASGamepadManager::poll()
{
    if (!m_sdl_ready) return;

    SDL_UpdateGamepads();

    if (m_gamepad && !SDL_GamepadConnected(m_gamepad)) closeGamepad();
    if (!m_gamepad)
    {
        if (++m_ticks_since_scan < SCAN_INTERVAL_TICKS) return;
        m_ticks_since_scan = 0;
        openGamepad();
        if (!m_gamepad) return;
    }

    auto held = [this](SDL_GamepadButton b) { return SDL_GetGamepadButton(m_gamepad, b); };
    auto axis = [this](SDL_GamepadAxis a) { return SDL_GetGamepadAxis(m_gamepad, a); };

    uint32_t buttons = 0;
    auto set = [&buttons](Button b, bool value) { buttons |= uint32_t(value) << b; };
    set(A, held(SDL_GAMEPAD_BUTTON_SOUTH));
    set(B, held(SDL_GAMEPAD_BUTTON_EAST));
    set(Z, held(SDL_GAMEPAD_BUTTON_WEST) || axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > AXIS_THRESHOLD);
    set(S, held(SDL_GAMEPAD_BUTTON_START));
    set(L, held(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
    set(R, held(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
    set(DU, held(SDL_GAMEPAD_BUTTON_DPAD_UP));
    set(DD, held(SDL_GAMEPAD_BUTTON_DPAD_DOWN));
    set(DL, held(SDL_GAMEPAD_BUTTON_DPAD_LEFT));
    set(DR, held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
    set(CU, axis(SDL_GAMEPAD_AXIS_RIGHTY) < -AXIS_THRESHOLD);
    set(CD, axis(SDL_GAMEPAD_AXIS_RIGHTY) > AXIS_THRESHOLD);
    set(CL, axis(SDL_GAMEPAD_AXIS_RIGHTX) < -AXIS_THRESHOLD);
    set(CR, axis(SDL_GAMEPAD_AXIS_RIGHTX) > AXIS_THRESHOLD);

    int x = remapAxis(axis(SDL_GAMEPAD_AXIS_LEFTX));
    int y = std::clamp(-remapAxis(axis(SDL_GAMEPAD_AXIS_LEFTY)), -128, 127);
    if (x * x + y * y < STICK_DEADZONE * STICK_DEADZONE) x = y = 0;

    const uint32_t changed = buttons ^ m_buttons;
    const bool stick_changed = x != m_stick_x || y != m_stick_y;
    m_buttons = buttons;
    m_stick_x = x;
    m_stick_y = y;

    if (changed)
    {
        buttonsChanged();
        if (m_enabled)
            for (int b = A; b <= CR; ++b)
                if ((changed >> b) & 1) buttonEdge(b, (buttons >> b) & 1);
    }
    if (stick_changed)
    {
        stickChanged();
        if (m_enabled) stickMoved(x, y);
    }
}
