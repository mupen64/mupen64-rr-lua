/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

class Plugin {
public:
    /**
     * \brief Tries to create a plugin from the given path
     * \param path The path to a plugin
     * \return The operation status along with a pointer to the plugin. The pointer will be invalid if the first pair element isn't an empty string.
     */
    static std::pair<std::wstring, std::unique_ptr<Plugin>> create(std::filesystem::path path);

    Plugin() = default;
    ~Plugin();

    /**
     * \brief Opens the plugin configuration dialog
     */
    void config();

    /**
     * \brief Opens the plugin test dialog
     */
    void test();

    /**
     * \brief Opens the plugin about dialog
     */
    void about();

    /**
     * \brief Loads the plugin's exported functions into the globals and calls the initiate function.
     */
    void initiate();

    /**
     * \brief Gets the plugin's path
     */
    auto path() const
    {
        return m_path;
    }

    /**
     * \brief Gets the plugin's name
     */
    auto name() const
    {
        return m_name;
    }

    /**
     * \brief Gets the plugin's type
     */
    auto type() const
    {
        return m_type;
    }

    /**
     * \brief Gets the plugin's version
     */
    auto version() const
    {
        return m_version;
    }

private:
    std::filesystem::path m_path;
    std::string m_name;
    core_plugin_type m_type;
    uint16_t m_version;
    HMODULE m_module;
};

/// <summary>
/// Initializes dummy info used by per-plugin functions
/// </summary>
void setup_dummy_info();
