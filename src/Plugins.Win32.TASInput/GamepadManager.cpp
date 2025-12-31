/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.h"
#include "GamepadManager.h"
#include <Main.h>
#include <NewConfig.h>

struct gamepad_manager_context
{
    SDL_Gamepad *gamepad{};
};

static gamepad_manager_context g_ctx;

static int32_t remap_axis(int16_t value, const bool is_y_axis)
{
    const int32_t min_target = is_y_axis ? -127 : -128;
    const int32_t max_target = is_y_axis ? 128 : 127;

    const int32_t mapped = static_cast<int32_t>(value) * max_target / 32767;

    return std::clamp(mapped, min_target, max_target);
}

void GamepadManager::on_sdl_event(const SDL_Event &e)
{
    switch (e.type)
    {
    case SDL_EVENT_GAMEPAD_ADDED:
        g_ctx.gamepad = SDL_OpenGamepad(e.gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (!g_ctx.gamepad) break;
        SDL_CloseGamepad(g_ctx.gamepad);
        g_ctx.gamepad = nullptr;
        break;
    default:
        break;
    }
}

static bool is_button_held(const t_button_mapping &mapping)
{
    if (mapping.button != SDL_GAMEPAD_BUTTON_INVALID)
    {
        return SDL_GetGamepadButton(g_ctx.gamepad, (SDL_GamepadButton)mapping.button) != 0;
    }

    if (mapping.key != 0)
    {
        return (GetAsyncKeyState(mapping.key) & 0x8000) != 0;
    }

    return false;
}

core_buttons GamepadManager::get_input(const size_t i)
{
    core_buttons buttons{};

    const auto controller_config = new_config.controller_config[i];
    
    buttons.a = is_button_held(controller_config.a);
    buttons.b = is_button_held(controller_config.b);
    buttons.z = is_button_held(controller_config.z);
    buttons.start = is_button_held(controller_config.start);
    buttons.l = is_button_held(controller_config.l);
    buttons.r = is_button_held(controller_config.r);

    buttons.du = is_button_held(controller_config.dpad_up);
    buttons.dd = is_button_held(controller_config.dpad_down);
    buttons.dl = is_button_held(controller_config.dpad_left);
    buttons.dr = is_button_held(controller_config.dpad_right);

    if (controller_config.x.axis == SDL_GAMEPAD_AXIS_INVALID)
    {
        const auto negative_held = GetAsyncKeyState(controller_config.x.key_negative) & 0x8000;
        const auto positive_held = GetAsyncKeyState(controller_config.x.key_positive) & 0x8000;

        if (controller_config.x.key_negative != 0 && negative_held)
        {
            buttons.x -= 128;
        }
        if (controller_config.x.key_positive != 0 && positive_held)
        {
            buttons.x += 127;
        }
    }
    else
    {
        buttons.x = remap_axis(SDL_GetGamepadAxis(g_ctx.gamepad, (SDL_GamepadAxis)controller_config.x.axis), false);
    }

    if (controller_config.y.axis == SDL_GAMEPAD_AXIS_INVALID)
    {
        const auto negative_held = GetAsyncKeyState(controller_config.y.key_negative) & 0x8000;
        const auto positive_held = GetAsyncKeyState(controller_config.y.key_positive) & 0x8000;

        if (controller_config.y.key_negative != 0 && negative_held)
        {
            buttons.y -= 127;
        }
        if (controller_config.y.key_positive != 0 && positive_held)
        {
            buttons.y += 128;
        }
    }
    else
    {
        buttons.y = remap_axis(SDL_GetGamepadAxis(g_ctx.gamepad, (SDL_GamepadAxis)controller_config.y.axis), true);
    }

    buttons.y *= -1;

    buttons.x = static_cast<int8_t>(buttons.x * controller_config.x_scale);
    buttons.y = static_cast<int8_t>(buttons.y * controller_config.y_scale);

    return buttons;
}
