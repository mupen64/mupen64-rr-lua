/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <SDL3/SDL_gamepad.h>
#include <nlohmann/json.hpp>

#define CONFIG_FILE_NAME "TASInput.json"

const auto AXIS_THRESHOLD = 16000;

struct t_axis_mapping
{
    int32_t axis = SDL_GAMEPAD_AXIS_INVALID;
    int32_t key_negative = 0;
    int32_t key_positive = 0;

    friend void to_json(nlohmann::json &j, const t_axis_mapping &self)
    {
#define TASINPUT_FIELD(field) {#field, self.field}
        j = nlohmann::json::object({
            TASINPUT_FIELD(axis),
            TASINPUT_FIELD(key_negative),
            TASINPUT_FIELD(key_positive),
        });
#undef TASINPUT_FIELD
    }

    friend void from_json(const nlohmann::json &j, t_axis_mapping &self)
    {
#define TASINPUT_FIELD(field) .field = j[#field]
        self = {
            TASINPUT_FIELD(axis),
            TASINPUT_FIELD(key_negative),
            TASINPUT_FIELD(key_positive),
        };
#undef TASINPUT_FIELD
    }
};

struct t_button_mapping
{
    int32_t button = SDL_GAMEPAD_BUTTON_INVALID;
    int32_t axis = SDL_GAMEPAD_AXIS_INVALID;
    int32_t key = 0;

    friend void to_json(nlohmann::json &j, const t_button_mapping &self)
    {
#define TASINPUT_FIELD(field) {#field, self.field}
        j = nlohmann::json::object({
            TASINPUT_FIELD(button),
            TASINPUT_FIELD(axis),
            TASINPUT_FIELD(key),
        });
#undef TASINPUT_FIELD
    }

    friend void from_json(const nlohmann::json &j, t_button_mapping &self)
    {
#define TASINPUT_FIELD(field) .field = j[#field]
        self = {
            TASINPUT_FIELD(button),
            TASINPUT_FIELD(axis),
            TASINPUT_FIELD(key),
        };
#undef TASINPUT_FIELD
    }
};

struct t_controller_config
{
    t_button_mapping dpad_right{};
    t_button_mapping dpad_left{};
    t_button_mapping dpad_down{};
    t_button_mapping dpad_up{};

    t_button_mapping c_right{};
    t_button_mapping c_left{};
    t_button_mapping c_down{};
    t_button_mapping c_up{};

    t_button_mapping a = {.button = SDL_GAMEPAD_BUTTON_SOUTH};
    t_button_mapping b = {.button = SDL_GAMEPAD_BUTTON_EAST};
    t_button_mapping z = {.button = SDL_GAMEPAD_BUTTON_WEST};
    t_button_mapping start = {.button = SDL_GAMEPAD_BUTTON_START};
    t_button_mapping l = {.button = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER};
    t_button_mapping r = {.button = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER};

    t_axis_mapping x = {.axis = SDL_GAMEPAD_AXIS_LEFTX};
    t_axis_mapping y = {.axis = SDL_GAMEPAD_AXIS_LEFTY};

    float x_scale = 1.0f;
    float y_scale = 1.0f;

    static t_controller_config keyboard_config()
    {
        t_controller_config config{};
        config.a = {.key = 'X'};
        config.b = {.key = 'Z'};
        config.start = {.key = VK_RETURN};
        config.z = {.key = 'A'};
        config.l = {.key = 'S'};
        config.r = {.key = 'D'};
        config.dpad_up = {.key = VK_UP};
        config.dpad_down = {.key = VK_DOWN};
        config.dpad_left = {.key = VK_LEFT};
        config.dpad_right = {.key = VK_RIGHT};
        config.c_up = {.key = 'W'};
        config.c_down = {.key = 'Q'};
        config.c_left = {.key = 'E'};
        config.c_right = {.key = 'R'};
        config.x = {.key_negative = 'J', .key_positive = 'L'};
        config.y = {.key_negative = 'I', .key_positive = 'K'};
        return config;
    }

    friend void to_json(nlohmann::json &j, const t_controller_config &self)
    {
#define TASINPUT_FIELD(field) {#field, self.field}
        j = nlohmann::json::object({
            TASINPUT_FIELD(dpad_right),
            TASINPUT_FIELD(dpad_left),
            TASINPUT_FIELD(dpad_down),
            TASINPUT_FIELD(dpad_up),
            TASINPUT_FIELD(c_right),
            TASINPUT_FIELD(c_left),
            TASINPUT_FIELD(c_down),
            TASINPUT_FIELD(c_up),
            TASINPUT_FIELD(a),
            TASINPUT_FIELD(b),
            TASINPUT_FIELD(z),
            TASINPUT_FIELD(start),
            TASINPUT_FIELD(l),
            TASINPUT_FIELD(r),
            TASINPUT_FIELD(x),
            TASINPUT_FIELD(y),
            TASINPUT_FIELD(x_scale),
            TASINPUT_FIELD(y_scale),
        });
#undef TASINPUT_FIELD
    }

    friend void from_json(const nlohmann::json &j, t_controller_config &self)
    {
#define TASINPUT_FIELD(field) .field = j[#field]
        self = {
            TASINPUT_FIELD(dpad_right), TASINPUT_FIELD(dpad_left), TASINPUT_FIELD(dpad_down), TASINPUT_FIELD(dpad_up),
            TASINPUT_FIELD(c_right),    TASINPUT_FIELD(c_left),    TASINPUT_FIELD(c_down),    TASINPUT_FIELD(c_up),
            TASINPUT_FIELD(a),          TASINPUT_FIELD(b),         TASINPUT_FIELD(z),         TASINPUT_FIELD(start),
            TASINPUT_FIELD(l),          TASINPUT_FIELD(r),         TASINPUT_FIELD(x),         TASINPUT_FIELD(y),
            TASINPUT_FIELD(x_scale),    TASINPUT_FIELD(y_scale),
        };
#undef TASINPUT_FIELD
    }
};

struct t_config
{
    int32_t version = 7;
    int32_t always_on_top = false;
    int32_t float_from_parent = true;
    int32_t titlebar = true;
    int32_t client_drag = true;
    int32_t dialog_expanded[4] = {0, 0, 0, 0};
    int32_t controller_active[4] = {1, 0, 0, 0};
    int32_t controller_mempak[4] = {0, 0, 0, 0};
    int32_t controller_rumblepak[4] = {0, 0, 0, 0};
    int32_t loop_combo = false;
    // Increments joystick position by the value of the magnitude slider when moving via keyboard or gamepad
    int32_t relative_mode = false;
    int32_t approach_mode = false;
    t_controller_config controller_config[4]{};
    uint64_t preferred_device_id{};

    friend void to_json(nlohmann::json &j, const t_config &self)
    {
#define TASINPUT_FIELD(field) {#field, self.field}
#define TASINPUT_ARRAY_FIELD(field) nlohmann::to_json(j[#field], self.field)
        j = nlohmann::json::object({
            TASINPUT_FIELD(version),
            TASINPUT_FIELD(always_on_top),
            TASINPUT_FIELD(float_from_parent),
            TASINPUT_FIELD(titlebar),
            TASINPUT_FIELD(client_drag),
            TASINPUT_FIELD(loop_combo),
            TASINPUT_FIELD(relative_mode),
            TASINPUT_FIELD(approach_mode),
        });
        TASINPUT_ARRAY_FIELD(dialog_expanded);
        TASINPUT_ARRAY_FIELD(controller_active);
        TASINPUT_ARRAY_FIELD(controller_mempak);
        TASINPUT_ARRAY_FIELD(controller_rumblepak);
        TASINPUT_ARRAY_FIELD(controller_config);
        TASINPUT_ARRAY_FIELD(preferred_device_id);
#undef TASINPUT_FIELD
#undef TASINPUT_ARRAY_FIELD
    }

    friend void from_json(const nlohmann::json &j, t_config &self)
    {
        if (!j.is_object()) throw std::domain_error("t_config expected JSON object");
#define TASINPUT_FIELD(field) nlohmann::from_json(j.at(#field), self.field)
        self = {};
        TASINPUT_FIELD(version);
        if (self.version >= 6)
        {
            TASINPUT_FIELD(always_on_top);
            TASINPUT_FIELD(float_from_parent);
            TASINPUT_FIELD(titlebar);
            TASINPUT_FIELD(client_drag);
            TASINPUT_FIELD(loop_combo);
            TASINPUT_FIELD(relative_mode);
            TASINPUT_FIELD(approach_mode);
            TASINPUT_FIELD(dialog_expanded);
            TASINPUT_FIELD(controller_active);
            TASINPUT_FIELD(controller_mempak);
            TASINPUT_FIELD(controller_rumblepak);
            TASINPUT_FIELD(controller_config);
            TASINPUT_FIELD(preferred_device_id);
        }
#undef TASINPUT_FIELD
    }
};

extern t_config new_config;

/**
 * \brief Saves the current config to a file
 */
void save_config();

/**
 * \brief Loads the config from a file
 */
void load_config();
