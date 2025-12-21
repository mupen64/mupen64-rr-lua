/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define SUBKEY L"Software\\N64 Emulation\\DLL\\TASDI"

struct t_controller_config {
    int32_t dpad_right = 0;
    int32_t dpad_left = 0;
    int32_t dpad_down = 0;
    int32_t dpad_up = 0;

    int32_t c_right = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    int32_t c_left = SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    int32_t c_down = SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    int32_t c_up = SDL_GAMEPAD_BUTTON_DPAD_UP;

    int32_t a = SDL_GAMEPAD_BUTTON_SOUTH;
    int32_t b = SDL_GAMEPAD_BUTTON_EAST;
    int32_t z = SDL_GAMEPAD_BUTTON_WEST;
    int32_t start = SDL_GAMEPAD_BUTTON_START;
    int32_t l = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    int32_t r = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;

    int32_t x = SDL_GAMEPAD_AXIS_LEFTX;
    int32_t y = SDL_GAMEPAD_AXIS_LEFTY;
};

typedef struct s_config {
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
