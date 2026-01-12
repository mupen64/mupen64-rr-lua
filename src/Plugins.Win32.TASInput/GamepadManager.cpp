/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
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

static int32_t remap_axis(int16_t value)
{
    const float v = static_cast<float>(value) / 32767.0f;
    const int32_t mapped = static_cast<int32_t>(std::lround(v * 128.0f));
    return std::clamp(mapped, -128, 127);
}

int8_t saturating_negate(int8_t v)
{
    if (v == -128) return 127;
    if (v == 127) return -128;
    return -v;
}

void GamepadManager::on_sdl_event(const SDL_Event &e)
{
    switch (e.type)
    {
    case SDL_EVENT_GAMEPAD_ADDED:
        if (g_ctx.gamepad)
        {
            SDL_CloseGamepad(g_ctx.gamepad);
            g_ctx.gamepad = nullptr;
        }
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
    if (mapping.axis != SDL_GAMEPAD_AXIS_INVALID)
    {
        if (g_ctx.gamepad == nullptr) return false;
        return std::abs(SDL_GetGamepadAxis(g_ctx.gamepad, (SDL_GamepadAxis)mapping.axis)) > AXIS_THRESHOLD;
    }

    if (mapping.button != SDL_GAMEPAD_BUTTON_INVALID)
    {
        if (g_ctx.gamepad == nullptr) return false;
        return SDL_GetGamepadButton(g_ctx.gamepad, (SDL_GamepadButton)mapping.button) != 0;
    }

    if (mapping.key != 0)
    {
        return (GetAsyncKeyState(mapping.key) & 0x8000) != 0;
    }

    return false;
}

static int32_t get_axis(const t_axis_mapping &mapping)
{
    if (mapping.axis == SDL_GAMEPAD_AXIS_INVALID)
    {
        const auto negative_held = GetAsyncKeyState(mapping.key_negative) & 0x8000;
        const auto positive_held = GetAsyncKeyState(mapping.key_positive) & 0x8000;

        if (mapping.key_negative != 0 && negative_held)
        {
            return -128;
        }
        if (mapping.key_positive != 0 && positive_held)
        {
            return 127;
        }
        return 0;
    }

    if (g_ctx.gamepad == nullptr) return 0;

    return remap_axis(SDL_GetGamepadAxis(g_ctx.gamepad, (SDL_GamepadAxis)mapping.axis));
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

    buttons.cu = is_button_held(controller_config.c_up);
    buttons.cd = is_button_held(controller_config.c_down);
    buttons.cl = is_button_held(controller_config.c_left);
    buttons.cr = is_button_held(controller_config.c_right);

    buttons.x = get_axis(controller_config.x);
    buttons.y = saturating_negate(get_axis(controller_config.y));

    buttons.x = static_cast<int8_t>(buttons.x * controller_config.x_scale);
    buttons.y = static_cast<int8_t>(buttons.y * controller_config.y_scale);

    return buttons;
}
