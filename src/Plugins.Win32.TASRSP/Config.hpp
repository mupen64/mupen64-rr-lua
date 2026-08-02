/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <nlohmann/json.hpp>

struct t_config
{
    int32_t version = 2;

    friend void to_json(nlohmann::json &j, const t_config &self)
    {
#define TASRSP_FIELD(field)                                                                                            \
    {                                                                                                                  \
        #field, self.field                                                                                             \
    }
        j = nlohmann::json::object({
            TASRSP_FIELD(version),
        });
#undef TASRSP_FIELD
    }

    friend void from_json(const nlohmann::json &j, t_config &self)
    {
        if (!j.is_object()) throw std::domain_error("t_config expected JSON object");
#define TASRSP_FIELD(field) nlohmann::from_json(j.at(#field), self.field)
        TASRSP_FIELD(version);
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
