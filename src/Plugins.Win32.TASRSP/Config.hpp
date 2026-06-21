/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <nlohmann/json.hpp>

struct t_config
{
    int32_t version = 2;
    /**
     * \brief Verify the cached ucode function on every audio ucode task. Enable this if you are debugging dynamic ucode
     * changes.
     */
    int32_t ucode_cache_verify = false;

    friend void to_json(nlohmann::json &j, const t_config &self)
    {
#define TASRSP_FIELD(field) {#field, self.field}
        j = nlohmann::json::object({
            TASRSP_FIELD(version),
            TASRSP_FIELD(ucode_cache_verify),
        });
#undef TASRSP_FIELD
    }

    friend void from_json(const nlohmann::json &j, t_config &self)
    {
        if (!j.is_object()) throw std::domain_error("t_config expected JSON object");
#define TASRSP_FIELD(field) nlohmann::from_json(j.at(#field), self.field)
        TASRSP_FIELD(version);
        TASRSP_FIELD(ucode_cache_verify);
#undef TASRSP_FIELD
    }
};

extern t_config config;

/**
 * \brief Saves the config
 */
void config_save();

/**
 * \brief Loads the config
 */
void config_load();

void config_show_dialog(HWND hwnd);
