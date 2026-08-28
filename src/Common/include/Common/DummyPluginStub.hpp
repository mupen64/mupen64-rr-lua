/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common/VersionNameHelpers.hpp>
#include <m64rr/Plugin.hpp>

#define DUMMY_PLUGIN_STUB_IMPL(plugin_type)                                                                            \
    EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)                                             \
    {                                                                                                                  \
        metadata->type = plugin_type;                                                                                  \
                                                                                                                       \
        const char *name = PLUGIN_NAME;                                                                                \
        const char *description = "Built-in plugin for Mupen64."                                                       \
                                  "\n\n"                                                                               \
                                  "https://mupen64.com";                                                               \
                                                                                                                       \
        auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);                        \
        metadata->name[result.size] = '\0';                                                                            \
                                                                                                                       \
        result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);        \
        metadata->description[result.size] = '\0';                                                                     \
    }
