/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

enum class AspectMode : uint8_t
{
    Pillarbox = 0,
    Stretch = 1,
    Widescreen = 2,
};

struct ResolutionPreset
{
    uint32_t width{};
    uint32_t height{};
    std::string description;
};

constexpr AspectMode DEFAULT_ASPECT_MODE = AspectMode::Widescreen;

const std::vector<ResolutionPreset> RESOLUTION_PRESETS = {
    {320, 240, "320 x 240 (4:3)"},     {400, 300, "400 x 300 (4:3)"},      {480, 360, "480 x 360 (4:3)"},
    {640, 480, "640 x 480 (4:3)"},     {800, 600, "800 x 600 (4:3)"},      {960, 720, "960 x 720 (4:3)"},
    {1024, 768, "1024 x 768 (4:3)"},   {1152, 864, "1152 x 864 (4:3)"},    {1280, 720, "1280 x 720 (16:9)"},
    {1280, 960, "1280 x 960 (4:3)"},   {1280, 1024, "1280 x 1024 (5:4)"},  {1440, 1080, "1440 x 1080 (4:3)"},
    {1600, 1200, "1600 x 1200 (4:3)"}, {1920, 1080, "1920 x 1080 (16:9)"}, {2560, 1440, "2560 x 1440 (16:9)"},
    {3840, 2160, "3840 x 2160 (16:9)"}};

const std::vector<std::pair<uint8_t, std::string>> FILTER_NAMES = {
    {0, "Default"},
    {1, "Always Smooth"},
    {2, "Always Pixelated"},
};

const std::vector<std::string> ASPECT_MODE_NAMES = {
    "Pillarbox",
    "Stretch",
    "Widescreen",
};

inline AspectMode to_aspect_mode(int value)
{
    if (value < 0 || value >= (int)ASPECT_MODE_NAMES.size()) return DEFAULT_ASPECT_MODE;
    return (AspectMode)value;
}

inline std::optional<ResolutionPreset> get_preset_by_resolution(uint32_t width, uint32_t height)
{
    for (const auto &preset : RESOLUTION_PRESETS)
    {
        if (preset.width == width && preset.height == height) return preset;
    }
    return std::nullopt;
}

void Config_LoadConfig();
void Config_SaveConfig();
