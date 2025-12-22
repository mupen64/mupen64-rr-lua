/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define SUBKEY L"Software\\N64 Emulation\\DLL\\TASDI"

struct t_axis_mapping
{
    int32_t axis = SDL_GAMEPAD_AXIS_INVALID;
    int32_t key_negative = SDL_SCANCODE_UNKNOWN;
    int32_t key_positive = SDL_SCANCODE_UNKNOWN;
};

struct t_button_mapping
{
    int32_t button = SDL_GAMEPAD_BUTTON_INVALID;
    int32_t key = SDL_SCANCODE_UNKNOWN;
};

struct t_controller_config
{
    t_button_mapping dpad_right = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};
    t_button_mapping dpad_left = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};
    t_button_mapping dpad_down = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};
    t_button_mapping dpad_up = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};

    t_button_mapping c_right = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};
    t_button_mapping c_left = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};
    t_button_mapping c_down = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};
    t_button_mapping c_up = {SDL_GAMEPAD_BUTTON_INVALID, SDL_SCANCODE_UNKNOWN};

    t_button_mapping a = {SDL_GAMEPAD_BUTTON_SOUTH, SDL_SCANCODE_UNKNOWN};
    t_button_mapping b = {SDL_GAMEPAD_BUTTON_EAST, SDL_SCANCODE_UNKNOWN};
    t_button_mapping z = {SDL_GAMEPAD_BUTTON_WEST, SDL_SCANCODE_UNKNOWN};
    t_button_mapping start = {SDL_GAMEPAD_BUTTON_START, SDL_SCANCODE_UNKNOWN};
    t_button_mapping l = {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_SCANCODE_UNKNOWN};
    t_button_mapping r = {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_SCANCODE_UNKNOWN};

    t_axis_mapping x = {SDL_GAMEPAD_AXIS_LEFTX, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN};
    t_axis_mapping y = {SDL_GAMEPAD_AXIS_LEFTY, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN};
};

typedef struct s_config
{
    int32_t version = 5;
    int32_t always_on_top = false;
    int32_t float_from_parent = true;
    int32_t titlebar = true;
    int32_t client_drag = true;
    int32_t dialog_expanded[4] = {0, 0, 0, 0};
    int32_t controller_active[4] = {1, 0, 0, 0};
    int32_t loop_combo = false;
    // Increments joystick position by the value of the magnitude slider when moving via keyboard or gamepad
    int32_t relative_mode = false;
    int32_t async_visual_updates = true;
    float x_scale[4] = {1, 1, 1, 1};
    float y_scale[4] = {1, 1, 1, 1};
    t_controller_config controller_config;
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
