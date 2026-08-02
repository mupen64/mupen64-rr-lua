/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <SDL3/SDL_gamepad.h>
#include <nlohmann/json.hpp>

#define CONFIG_FILE_NAME "TASInput.json"

const auto AXIS_THRESHOLD = 16000;

inline void to_json(nlohmann::json &j, const SDL_GUID &guid)
{
    char buffer[33];
    SDL_GUIDToString(guid, buffer, sizeof(buffer));
    j = buffer;
}

inline void from_json(const nlohmann::json &j, SDL_GUID &guid)
{
    if (j.is_string())
    {
        std::string guid_str = j.get<std::string>();
        guid = SDL_StringToGUID(guid_str.c_str());
    }
    else
    {
        std::memset(&guid, 0, sizeof(guid));
    }
}

inline bool operator==(const SDL_GUID &a, const SDL_GUID &b)
{
    return std::memcmp(a.data, b.data, sizeof(a.data)) == 0;
}

struct t_axis_mapping
{
    int32_t axis = SDL_GAMEPAD_AXIS_INVALID;
    int32_t key_negative = 0;
    int32_t key_positive = 0;

    friend void to_json(nlohmann::json &j, const t_axis_mapping &self)
    {
#define TASINPUT_FIELD(field)                                                                                          \
    {                                                                                                                  \
        #field, self.field                                                                                             \
    }
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
#define TASINPUT_FIELD(field)                                                                                          \
    {                                                                                                                  \
        #field, self.field                                                                                             \
    }
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

    t_button_mapping a{};
    t_button_mapping b{};
    t_button_mapping z{};
    t_button_mapping start{};
    t_button_mapping l{};
    t_button_mapping r{};

    t_axis_mapping x{};
    t_axis_mapping y{};

    float x_scale = 1.0f;
    float y_scale = 1.0f;

    static t_controller_config gamepad_config()
    {
        t_controller_config config{};
        config.a = {.button = SDL_GAMEPAD_BUTTON_SOUTH};
        config.b = {.button = SDL_GAMEPAD_BUTTON_EAST};
        config.start = {.button = SDL_GAMEPAD_BUTTON_START};
        config.z = {.button = SDL_GAMEPAD_BUTTON_WEST};
        config.l = {.button = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER};
        config.r = {.button = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER};
        config.dpad_up = {};
        config.dpad_down = {};
        config.dpad_left = {};
        config.dpad_right = {};
        config.c_up = {};
        config.c_down = {};
        config.c_left = {};
        config.c_right = {};
        config.x = {.axis = SDL_GAMEPAD_AXIS_LEFTX};
        config.y = {.axis = SDL_GAMEPAD_AXIS_LEFTY};
        return config;
    }

    static t_controller_config keyboard_config()
    {
        t_controller_config config{};
        config.a = {.key = VK_SPACE};
        config.b = {.key = 'B'};
        config.start = {.key = VK_RETURN};
        config.z = {.key = 'Z'};
        config.l = {.key = 'L'};
        config.r = {.key = 'R'};
        config.dpad_up = {};
        config.dpad_down = {};
        config.dpad_left = {};
        config.dpad_right = {};
        config.c_up = {.key = VK_UP};
        config.c_down = {.key = VK_DOWN};
        config.c_left = {.key = VK_LEFT};
        config.c_right = {.key = VK_RIGHT};
        config.x = {.key_negative = 'A', .key_positive = 'D'};
        config.y = {.key_negative = 'W', .key_positive = 'S'};
        return config;
    }

    friend void to_json(nlohmann::json &j, const t_controller_config &self)
    {
#define TASINPUT_FIELD(field)                                                                                          \
    {                                                                                                                  \
        #field, self.field                                                                                             \
    }
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
    t_controller_config controller_config[4] = {t_controller_config::keyboard_config(), {}, {}, {}};
    std::optional<SDL_GUID> preferred_device_guid;

    friend void to_json(nlohmann::json &j, const t_config &self)
    {
#define TASINPUT_FIELD(field)                                                                                          \
    {                                                                                                                  \
        #field, self.field                                                                                             \
    }
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
            TASINPUT_FIELD(preferred_device_guid),
        });
        TASINPUT_ARRAY_FIELD(dialog_expanded);
        TASINPUT_ARRAY_FIELD(controller_active);
        TASINPUT_ARRAY_FIELD(controller_mempak);
        TASINPUT_ARRAY_FIELD(controller_rumblepak);
        TASINPUT_ARRAY_FIELD(controller_config);
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
            TASINPUT_FIELD(preferred_device_guid);
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
