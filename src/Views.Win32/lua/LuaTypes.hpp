/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/ActionManager.hpp>
#include <SDL3/SDL_keycode.h>

#include <lua/presenters/Presenter.hpp>
#include <memory>

class LuaEnvironment;

struct ActionParamMeta
{
    uintptr_t *validator{};
    uintptr_t *get_initial_value{};
    uintptr_t *get_hints{};
};



/**
 * \brief Represents the arguments for a key event callback. See `KeyEventArgs` in `api.lua`.
 */
struct LuaKeyEventArgs
{
    std::optional<uint64_t> keycode;
    std::optional<SDL_Keycode> keycode2;
    bool ctrl{};
    bool alt{};
    bool shift{};
    bool meta{};
    std::optional<bool> pressed;
    std::optional<std::string> text;
    bool repeat{};
};

using LuaMouseButton = uint32_t;

struct LuaMouseEventArgs
{
    int32_t x{};
    int32_t y{};
    bool ctrl{};
    bool alt{};
    bool shift{};
    bool meta{};
    std::optional<int32_t> x_wheel;
    std::optional<int32_t> y_wheel;
    std::optional<LuaMouseButton> button;
    std::optional<bool> pressed;
    std::optional<bool> double_click;
    std::optional<bool> triple_click;
};
