/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define SUBKEY L"Software\\N64 Emulation\\DLL\\TASDI"

const auto AXIS_THRESHOLD = 16000;

struct t_axis_mapping
{
    int32_t axis = SDL_GAMEPAD_AXIS_INVALID;
    int32_t key_negative = 0;
    int32_t key_positive = 0;
};

struct t_button_mapping
{
    int32_t button = SDL_GAMEPAD_BUTTON_INVALID;
    int32_t axis = SDL_GAMEPAD_AXIS_INVALID;
    int32_t key = 0;

    static t_button_mapping from_button(int32_t button) { return {button, SDL_GAMEPAD_AXIS_INVALID, 0}; }
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

    t_button_mapping a = t_button_mapping::from_button(SDL_GAMEPAD_BUTTON_SOUTH);
    t_button_mapping b = t_button_mapping::from_button(SDL_GAMEPAD_BUTTON_EAST);
    t_button_mapping z = t_button_mapping::from_button(SDL_GAMEPAD_BUTTON_WEST);
    t_button_mapping start = t_button_mapping::from_button(SDL_GAMEPAD_BUTTON_START);
    t_button_mapping l = t_button_mapping::from_button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    t_button_mapping r = t_button_mapping::from_button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);

    t_axis_mapping x = {SDL_GAMEPAD_AXIS_LEFTX, 0, 0};
    t_axis_mapping y = {SDL_GAMEPAD_AXIS_LEFTY, 0, 0};

    float x_scale = 1.0f;
    float y_scale = 1.0f;

    static t_controller_config keyboard_config()
    {
        t_controller_config config{};
        config.a = {SDL_GAMEPAD_BUTTON_INVALID, 'X'};
        config.b = {SDL_GAMEPAD_BUTTON_INVALID, 'Z'};
        config.start = {SDL_GAMEPAD_BUTTON_INVALID, VK_RETURN};
        config.z = {SDL_GAMEPAD_BUTTON_INVALID, 'A'};
        config.l = {SDL_GAMEPAD_BUTTON_INVALID, 'S'};
        config.r = {SDL_GAMEPAD_BUTTON_INVALID, 'D'};
        config.dpad_up = {SDL_GAMEPAD_BUTTON_INVALID, VK_UP};
        config.dpad_down = {SDL_GAMEPAD_BUTTON_INVALID, VK_DOWN};
        config.dpad_left = {SDL_GAMEPAD_BUTTON_INVALID, VK_LEFT};
        config.dpad_right = {SDL_GAMEPAD_BUTTON_INVALID, VK_RIGHT};
        config.c_up = {SDL_GAMEPAD_BUTTON_INVALID, 'W'};
        config.c_down = {SDL_GAMEPAD_BUTTON_INVALID, 'Q'};
        config.c_left = {SDL_GAMEPAD_BUTTON_INVALID, 'E'};
        config.c_right = {SDL_GAMEPAD_BUTTON_INVALID, 'R'};
        config.x = {SDL_GAMEPAD_AXIS_INVALID, 'J', 'L'};
        config.y = {SDL_GAMEPAD_AXIS_INVALID, 'I', 'K'};
        return config;
    }
};

typedef struct s_config
{
    int32_t version = 6;
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
} t_config;

extern t_config new_config;

/**
 * \brief Saves the current config to a file
 */
void save_config();

/**
 * \brief Loads the config from a file
 */
void load_config();
