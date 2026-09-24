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
    SDL_Joystick *joystick{};
    std::optional<SDL_JoystickID> gamepad_id;
    std::optional<SDL_GUID> guid;
    std::optional<std::string> path;
    bool is_gamepad{};
    GamepadManager::DeviceRegistry reg{};
};

static gamepad_manager_context g_ctx;

static void refresh_registry()
{
    g_ctx.reg.devices.clear();

    g_ctx.reg.devices.emplace_back(GamepadManager::InputDevice{
        .type = GamepadManager::InputDeviceType::Keyboard,
        .instance_id = std::nullopt,
        .guid = std::nullopt,
        .path = std::nullopt,
        .name = "Keyboard",
    });

    int32_t count{};
    SDL_JoystickID *joy_ids = SDL_GetJoysticks(&count);
    if (!joy_ids) return;

    for (int32_t i = 0; i < count; ++i)
    {
        const auto id = joy_ids[i];
        const bool is_gamepad = SDL_IsGamepad(id);
        const char *name = SDL_GetJoystickNameForID(id);
        const char *path = SDL_GetJoystickPathForID(id);

        g_ctx.reg.devices.emplace_back(GamepadManager::InputDevice{
            .type = is_gamepad ? GamepadManager::InputDeviceType::Gamepad : GamepadManager::InputDeviceType::Joystick,
            .instance_id = id,
            .guid = SDL_GetJoystickGUIDForID(id),
            .path = path ? std::optional<std::string>(path) : std::nullopt,
            .name = is_gamepad ? (name ? name : "Unknown gamepad")
                               : std::format("{} (Joystick)", name ? name : "Unknown controller"),
        });
    }
    SDL_free(joy_ids);
}

static void close_gamepad()
{
    if (g_ctx.gamepad)
        SDL_CloseGamepad(g_ctx.gamepad);
    else if (g_ctx.joystick)
        SDL_CloseJoystick(g_ctx.joystick);

    g_ctx.gamepad = nullptr;
    g_ctx.joystick = nullptr;
    g_ctx.gamepad_id.reset();
    g_ctx.guid.reset();
    g_ctx.path.reset();
    g_ctx.is_gamepad = false;
}

static void log_guid(const char *action, SDL_GUID guid, bool is_gamepad)
{
    char guid_string[33]{};
    SDL_GUIDToString(guid, guid_string, sizeof(guid_string));
    g_plugin->log_info(std::format("{} {} {}", action, is_gamepad ? "gamepad" : "joystick", guid_string).c_str());
}

static bool open_device(SDL_JoystickID instance_id)
{
    close_gamepad();

    g_ctx.is_gamepad = SDL_IsGamepad(instance_id);
    if (g_ctx.is_gamepad)
    {
        g_ctx.gamepad = SDL_OpenGamepad(instance_id);
        if (g_ctx.gamepad) g_ctx.joystick = SDL_GetGamepadJoystick(g_ctx.gamepad);
    }
    else
    {
        g_ctx.joystick = SDL_OpenJoystick(instance_id);
    }

    if (!g_ctx.joystick)
    {
        g_ctx.gamepad = nullptr;
        g_ctx.is_gamepad = false;
        return false;
    }

    g_ctx.gamepad_id = instance_id;
    g_ctx.guid = SDL_GetJoystickGUID(g_ctx.joystick);
    if (const char *path = SDL_GetJoystickPath(g_ctx.joystick)) g_ctx.path = path;
    log_guid("Opened", *g_ctx.guid, g_ctx.is_gamepad);
    return true;
}

static std::optional<SDL_JoystickID> find_configured_device()
{
    int count{};
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (!ids) return std::nullopt;

    std::optional<SDL_JoystickID> result;
    for (int i = 0; i < count; ++i)
    {
        if (new_config.preferred_device_path.has_value())
        {
            const char *path = SDL_GetJoystickPathForID(ids[i]);
            if (path && *new_config.preferred_device_path == path)
            {
                result = ids[i];
                break;
            }
        }
        else if (new_config.preferred_device_guid.has_value() &&
                 SDL_GetJoystickGUIDForID(ids[i]) == *new_config.preferred_device_guid)
        {
            result = ids[i];
            break;
        }
    }

    SDL_free(ids);
    return result;
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
    case SDL_EVENT_JOYSTICK_ADDED:
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_KEYBOARD_ADDED:
    case SDL_EVENT_KEYBOARD_REMOVED:
        refresh_registry_and_update_gamepad();
        break;
    case SDL_EVENT_JOYSTICK_REMOVED:
        if (g_ctx.gamepad_id == e.jdevice.which) close_gamepad();
        refresh_registry_and_update_gamepad();
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (g_ctx.gamepad_id == e.gdevice.which) close_gamepad();
        refresh_registry_and_update_gamepad();
        break;
    default:
        break;
    }
}

static int16_t get_device_axis(int32_t axis)
{
    if (!g_ctx.joystick) return 0;
    if (g_ctx.is_gamepad) return SDL_GetGamepadAxis(g_ctx.gamepad, static_cast<SDL_GamepadAxis>(axis));
    return SDL_GetJoystickAxis(g_ctx.joystick, axis);
}

static bool get_device_button(int32_t button)
{
    if (!g_ctx.joystick) return false;
    if (g_ctx.is_gamepad) return SDL_GetGamepadButton(g_ctx.gamepad, static_cast<SDL_GamepadButton>(button));
    return SDL_GetJoystickButton(g_ctx.joystick, button);
}

static bool is_button_held(const ButtonMapping &mapping)
{
    if (mapping.hat >= 0)
    {
        if (!g_ctx.joystick || g_ctx.is_gamepad || mapping.hat_mask == SDL_HAT_CENTERED) return false;
        return (SDL_GetJoystickHat(g_ctx.joystick, mapping.hat) & mapping.hat_mask) == mapping.hat_mask;
    }

    if (mapping.axis != SDL_GAMEPAD_AXIS_INVALID)
    {
        const auto value = get_device_axis(mapping.axis);
        if (mapping.axis_direction < 0) return value < -AXIS_THRESHOLD;
        if (mapping.axis_direction > 0) return value > AXIS_THRESHOLD;
        return std::abs(value) > AXIS_THRESHOLD;
    }

    if (mapping.button != SDL_GAMEPAD_BUTTON_INVALID) return get_device_button(mapping.button);

    if (mapping.key != 0) return (GetAsyncKeyState(mapping.key) & 0x8000) != 0;

    return false;
}

static int32_t get_axis(const AxisMapping &mapping)
{
    if (mapping.axis == SDL_GAMEPAD_AXIS_INVALID)
    {
        const auto negative_held = GetAsyncKeyState(mapping.key_negative) & 0x8000;
        const auto positive_held = GetAsyncKeyState(mapping.key_positive) & 0x8000;

        if (mapping.key_negative != 0 && negative_held) return -128;
        if (mapping.key_positive != 0 && positive_held) return 127;
        return 0;
    }

    return remap_axis(get_device_axis(mapping.axis));
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

    if (is_button_held(controller_config.mag1))
    {
        float stick_mag = sqrtf(static_cast<float>(buttons.x * buttons.x + buttons.y * buttons.y));
        if (stick_mag > 0.0f && controller_config.mag1_val < 128)
        {
            buttons.x = static_cast<int8_t>(static_cast<float>(buttons.x) * controller_config.mag1_val / stick_mag);
            buttons.y = static_cast<int8_t>(static_cast<float>(buttons.y) * controller_config.mag1_val / stick_mag);
        }
    }

    if (is_button_held(controller_config.mag2) && !is_button_held(controller_config.mag1))
    {
        float stick_mag = sqrtf(static_cast<float>(buttons.x * buttons.x + buttons.y * buttons.y));
        if (stick_mag > 0.0f && controller_config.mag2_val < 128)
        {
            buttons.x = static_cast<int8_t>(static_cast<float>(buttons.x) * controller_config.mag2_val / stick_mag);
            buttons.y = static_cast<int8_t>(static_cast<float>(buttons.y) * controller_config.mag2_val / stick_mag);
        }
    }

    return buttons;
}

void GamepadManager::update_current_gamepad()
{
    if (g_ctx.joystick)
    {
        const bool connected = SDL_JoystickConnected(g_ctx.joystick);
        const bool same_device =
            new_config.preferred_device_path.has_value()
                ? g_ctx.path == new_config.preferred_device_path
                : new_config.preferred_device_guid.has_value() && g_ctx.guid == new_config.preferred_device_guid;
        const bool same_api =
            connected && g_ctx.gamepad_id.has_value() && g_ctx.is_gamepad == SDL_IsGamepad(*g_ctx.gamepad_id);
        if (connected && same_device && same_api) return;

        if (g_ctx.guid.has_value()) log_guid("Closing", *g_ctx.guid, g_ctx.is_gamepad);
        close_gamepad();
    }

    if (!new_config.preferred_device_path.has_value() && !new_config.preferred_device_guid.has_value()) return;

    const auto id = find_configured_device();
    if (!id.has_value())
    {
        g_plugin->log_warn("Configured input device is not connected");
        return;
    }

    if (!open_device(*id))
    {
        g_plugin->log_warn(std::format("Failed to open input device: {}", SDL_GetError()).c_str());
        return;
    }

    if (!new_config.preferred_device_path.has_value() && g_ctx.path.has_value())
        new_config.preferred_device_path = g_ctx.path;
}

void GamepadManager::select_gamepad(SDL_JoystickID instance_id)
{
    new_config.preferred_device_guid = SDL_GetJoystickGUIDForID(instance_id);
    if (const char *path = SDL_GetJoystickPathForID(instance_id))
        new_config.preferred_device_path = path;
    else
        new_config.preferred_device_path.reset();

    if (!open_device(instance_id))
        g_plugin->log_warn(std::format("Failed to open selected input device: {}", SDL_GetError()).c_str());
}

std::optional<SDL_JoystickID> GamepadManager::current_gamepad_id()
{
    return g_ctx.gamepad_id;
}

bool GamepadManager::current_device_is_gamepad()
{
    return g_ctx.joystick && g_ctx.is_gamepad;
}

void GamepadManager::shutdown()
{
    close_gamepad();
}

GamepadManager::DeviceRegistry &GamepadManager::device_registry()
{
    if (g_ctx.reg.devices.empty()) refresh_registry_and_update_gamepad();
    return g_ctx.reg;
}
