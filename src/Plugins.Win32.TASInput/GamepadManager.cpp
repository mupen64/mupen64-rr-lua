/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "GamepadManager.hpp"
#include <Main.hpp>
#include <NewConfig.hpp>

struct gamepad_manager_context
{
    SDL_Gamepad *gamepad{};
    GamepadManager::DeviceRegistry reg{};
};

static gamepad_manager_context g_ctx;

static SDL_GUID SDL_GetGamepadGUID(SDL_Gamepad *gamepad)
{
    const auto joystick = SDL_GetGamepadJoystick(gamepad);
    return SDL_GetJoystickGUID(joystick);
}

SDL_Gamepad *SDL_OpenGamepadByGUID(SDL_GUID target_guid)
{
    int count = 0;
    SDL_JoystickID *gamepad_ids = SDL_GetGamepads(&count);

    if (!gamepad_ids) return NULL;

    SDL_Gamepad *opened_gamepad = NULL;

    for (int i = 0; i < count; i++)
    {
        SDL_JoystickID instance_id = gamepad_ids[i];
        SDL_GUID current_guid = SDL_GetGamepadGUIDForID(instance_id);
        if (SDL_memcmp(&current_guid, &target_guid, sizeof(SDL_GUID)) == 0)
        {
            opened_gamepad = SDL_OpenGamepad(instance_id);
            break;
        }
    }

    SDL_free(gamepad_ids);
    return opened_gamepad;
}

static void refresh_registry()
{
    g_ctx.reg.devices.clear();

    g_ctx.reg.devices.emplace_back(GamepadManager::InputDevice{
        .type = GamepadManager::InputDeviceType::Keyboard,
        .guid = std::nullopt,
        .name = "Keyboard",
    });

    int32_t count{};
    const SDL_JoystickID *joy_ids = SDL_GetGamepads(&count);
    for (int32_t i = 0; i < count; ++i)
    {
        const auto id = joy_ids[i];
        const auto name = SDL_GetJoystickNameForID(id);

        g_ctx.reg.devices.emplace_back(GamepadManager::InputDevice{
            .type = GamepadManager::InputDeviceType::Gamepad,
            .guid = SDL_GetJoystickGUIDForID(id),
            .name = name,
        });
    }
    SDL_free((void *)joy_ids);
}

static void refresh_registry_and_update_gamepad()
{
    refresh_registry();
    GamepadManager::update_current_gamepad();
}

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
    case SDL_EVENT_GAMEPAD_REMOVED:
    case SDL_EVENT_KEYBOARD_ADDED:
    case SDL_EVENT_KEYBOARD_REMOVED:
        refresh_registry_and_update_gamepad();
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

CoreButtons GamepadManager::get_input(const size_t i)
{
    CoreButtons buttons{};

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

    if (is_button_held(controller_config.mag1)) {
        int32_t buttons_x = buttons.x;
        int32_t buttons_y = buttons.y;
        float stick_mag = sqrtf(static_cast<float>(buttons.x * buttons.x + buttons.y * buttons.y));
        if (stick_mag > 0.0f && controller_config.mag1_val < 127) {
            buttons.x = static_cast<int8_t>(static_cast<float>(buttons.x) * controller_config.mag1_val / stick_mag);
            buttons.y = static_cast<int8_t>(static_cast<float>(buttons.y) * controller_config.mag1_val / stick_mag);
        }
    }

    if (is_button_held(controller_config.mag2) && !is_button_held(controller_config.mag1)) {
        float stick_mag = sqrtf(static_cast<float>(buttons.x * buttons.x + buttons.y * buttons.y));
        if (stick_mag > 0.0f && controller_config.mag2_val < 127) {
            buttons.x = static_cast<int8_t>(static_cast<float>(buttons.x) * controller_config.mag2_val / stick_mag);
            buttons.y = static_cast<int8_t>(static_cast<float>(buttons.y) * controller_config.mag2_val / stick_mag);
        }
    }
    
    return buttons;
}

void GamepadManager::update_current_gamepad()
{
    if (g_ctx.gamepad)
    {
        if (SDL_GetGamepadGUID(g_ctx.gamepad) == new_config.preferred_device_guid) return;
        g_plugin->log_info(std::format("Closing gamepad {}", SDL_GetGamepadGUID(g_ctx.gamepad).data).c_str());
        SDL_CloseGamepad(g_ctx.gamepad);
        g_ctx.gamepad = nullptr;
    }

    if (!new_config.preferred_device_guid.has_value()) return;

    g_ctx.gamepad = SDL_OpenGamepadByGUID(*new_config.preferred_device_guid);
    if (!g_ctx.gamepad)
    {
        g_plugin->log_info(std::format("Failed to open gamepad {}", *new_config.preferred_device_guid->data).c_str());
        return;
    }
    g_plugin->log_info(std::format("Opened gamepad {}", *new_config.preferred_device_guid->data).c_str());
}

GamepadManager::DeviceRegistry &GamepadManager::device_registry()
{
    if (g_ctx.reg.devices.empty()) refresh_registry_and_update_gamepad();
    return g_ctx.reg;
}
