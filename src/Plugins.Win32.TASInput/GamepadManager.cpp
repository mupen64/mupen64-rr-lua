/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#include "stdafx.h"
#include "GamepadManager.h"
#include <Main.h>

struct gamepad_manager_context {
    SDL_Gamepad* gamepad{};
};

static gamepad_manager_context g_ctx;

static int32_t remap_axis(int16_t value, const bool is_y_axis)
{
    const int32_t min_target = is_y_axis ? -127 : -128;
    const int32_t max_target = is_y_axis ? 128 : 127;

    const int32_t mapped = static_cast<int32_t>(value) * max_target / 32767;

    if (is_y_axis)
    {
        g_ef->log_trace(std::format(L"y: {}", std::clamp(mapped, min_target, max_target)).c_str());
    }
    else
    {
        g_ef->log_trace(std::format(L"x: {}", std::clamp(mapped, min_target, max_target)).c_str());
    }
    
    return std::clamp(mapped, min_target, max_target);
}

void GamepadManager::init()
{
    SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT_CORRELATE_XINPUT, "0");
    RT_ASSERT(SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK), L"Failed to initialize SDL subsystems");
}

void GamepadManager::poll_events()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_EVENT_GAMEPAD_ADDED:
            {
                g_ctx.gamepad = SDL_OpenGamepad(e.gdevice.which);
                break;
            }
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (g_ctx.gamepad)
            {
                SDL_CloseGamepad(g_ctx.gamepad);
                g_ctx.gamepad = nullptr;
            }
            break;
        default:
            break;
        }
    }
}

core_buttons GamepadManager::get_input()
{
    core_buttons buttons{};

    if (!g_ctx.gamepad)
        return buttons;

    buttons.a = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
    buttons.b = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_EAST);
    buttons.z = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_WEST);
    buttons.start = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_START);
    buttons.l = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    buttons.r = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);

    buttons.du = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
    buttons.dd = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    buttons.dl = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    buttons.dr = SDL_GetGamepadButton(g_ctx.gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

    buttons.x = remap_axis(SDL_GetGamepadAxis(g_ctx.gamepad, SDL_GAMEPAD_AXIS_LEFTX), false);
    buttons.y = -remap_axis(SDL_GetGamepadAxis(g_ctx.gamepad, SDL_GAMEPAD_AXIS_LEFTY), true);

    return buttons;
}
