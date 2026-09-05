/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Config.hpp"
#include "TASVideo.hpp"
#include "RSP.hpp"
#include "Textures.h"
#include "OpenGL.hpp"
#include <nlohmann/json.hpp>
using nlohmann::json;

#define CONFIG_FILE_NAME "TASVideo.json"

static std::filesystem::path get_config_path()
{
    const auto size = g_plugin->config_path(nullptr, 0);
    std::string path(size - 1, '\0');
    g_plugin->config_path(path.data(), size);
    return std::filesystem::path(path) / CONFIG_FILE_NAME;
}

static void Config_SetDefaults()
{
    OGL.smoothing = 0;
    OGL.fog = TRUE;
    OGL.msaa = 0;
    OGL.windowedWidth = 640;
    OGL.windowedHeight = 480;
    cache.maxBytes = 32 * 1048576;
    OGL.textureFilter = TextureFilter::None;
    OGL.usePolygonStipple = FALSE;
    OGL.aspectMode = default_aspect_mode;
}

void Config_LoadConfig()
{
    auto json_path = get_config_path();

    if (!std::filesystem::exists(json_path))
    {
        Config_SetDefaults();
        Config_SaveConfig();
        return;
    }

    std::ifstream ifs(json_path);
    nlohmann::json j;

    try
    {
        ifs >> j;
        OGL.windowedWidth = j["windowed_width"];
        OGL.windowedHeight = j["windowed_height"];
        OGL.smoothing = j["smoothing"];
        OGL.textureFilter = j["texture_filter"];
        OGL.filterScale = j["filter_scale"];
        OGL.fog = j["enable_fog"];
        OGL.msaa = (j.value("msaa", 0) == 4) ? 4 : 0;
        cache.maxBytes = (int)j["texture_cache_size"] * 1048576;
        OGL.usePolygonStipple = j["dithered_alpha_testing"];
        OGL.ignoreScissor = j["ignore_scissor"];
        OGL.clear_override = j["clear_override"];
        OGL.aspectMode = to_aspect_mode(j.value("aspect_mode", (int)default_aspect_mode));
    }
    catch (const std::exception &e)
    {
        g_plugin->log_warn("Config load failed, using defaults...");
        Config_SetDefaults();
    }
}

void Config_SaveConfig()
{
    json j = json::object({
        {"windowed_width", OGL.windowedWidth},
        {"windowed_height", OGL.windowedHeight},
        {"smoothing", OGL.smoothing},
        {"texture_filter", OGL.textureFilter},
        {"filter_scale", OGL.filterScale},
        {"enable_fog", (bool)OGL.fog},
        {"msaa", OGL.msaa},
        {"texture_cache_size", cache.maxBytes / 1048576},
        {"dithered_alpha_testing", (bool)OGL.usePolygonStipple},
        {"ignore_scissor", (bool)OGL.ignoreScissor},
        {"clear_override", (bool)OGL.clear_override},
        {"aspect_mode", (int)OGL.aspectMode},
    });

    std::ofstream ofs(get_config_path());
    ofs << std::setw(2) << j;
}
