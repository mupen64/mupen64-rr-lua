/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <nlohmann/json.hpp>

struct InputPortConfig
{
    bool present = false;

    friend void to_json(nlohmann::json &j, const InputPortConfig &self);
    friend void from_json(const nlohmann::json &j, InputPortConfig &self);
};

struct InputConfig
{
    int32_t version = 2;

    std::array<InputPortConfig, 4> ports{
        InputPortConfig{.present = true},
        InputPortConfig{},
        InputPortConfig{},
        InputPortConfig{},
    };

    friend void to_json(nlohmann::json &j, const InputConfig &self);
    friend void from_json(const nlohmann::json &j, InputConfig &self);
};

extern InputConfig g_tasinput_config;

/**
 * \brief Saves the config
 */
void tasinput_config_save();

/**
 * \brief Loads the config
 */
void tasinput_config_load();
