/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <VersionNameHelpers.hpp>
#include <m64rr/Plugin.hpp>

#define DUMMY_PLUGIN_STUB_IMPL(plugin_type)                                                                            \
    EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)                                             \
    {                                                                                                                  \
        metadata->type = plugin_type;                                                                                  \
                                                                                                                       \
        const auto name = IOUtils::to_utf8_string(PLUGIN_NAME);                                                        \
        const auto description = "First-party TAS plugin for Mupen64."                                                 \
                                 "\n"                                                                                  \
                                 "TAS plugins are not to be distributed separately from Mupen64 and remain tied "      \
                                 "to one version of the emulator."                                                     \
                                 "\n\n"                                                                                \
                                 "https://mupen64.com";                                                                \
        const auto target_version = IOUtils::to_utf8_string(CURRENT_VERSION);                                          \
                                                                                                                       \
        auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);                        \
        metadata->name[result.size] = '\0';                                                                            \
                                                                                                                       \
        result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);        \
        metadata->description[result.size] = '\0';                                                                     \
                                                                                                                       \
        result =                                                                                                       \
            std::format_to_n(metadata->target_version, sizeof(metadata->target_version) - 1, "{}", target_version);    \
        metadata->target_version[result.size] = '\0';                                                                  \
    }\
\
