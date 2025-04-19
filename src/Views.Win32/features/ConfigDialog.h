/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing configuration dialogs.
 */
namespace ConfigDialog
{
    /**
     * \brief Initializes the subsystem.
     */
    void init();

    /**
     * \brief Shows the application settings dialog.
     */
    void show_app_settings();

    /**
     * \brief Shows the plugin settings dialog.
     * \param plugin The plugin being configured.
     * \param cfg The plugin configuration.
     * \return Whether the user chose to save the settings.
     */
    bool show_plugin_settings(Plugin* plugin, core_plugin_cfg* cfg);

    /**
     * \brief Shows the about dialog.
     */
    void show_about();
} // namespace ConfigDialog
