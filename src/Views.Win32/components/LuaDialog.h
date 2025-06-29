/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing the Lua manager dialog and related functionality.
 */
namespace LuaDialog
{
    /**
     * \brief Shows the Lua manager dialog.
     */
    void show();

    /**
     * \brief Adds a lua script to the Lua instance manager and starts it. Also shows the Lua manager dialog and selects the new script in the list.
     * \param path The path to the lua script to add and run.
     */
    void add_and_start(const std::filesystem::path& path);

    /**
     * \brief Stops all running Lua scripts.
     */
    void stop_all();

    /**
     * \brief Stops all Lua scripts and removes all entries from the Lua instance manager.
     */
    void close_all();

    /**
     * \brief Stores a list of all running Lua scripts to be recalled later using <c>load_running_scripts</c>.
     */
    void store_running_scripts();

    /**
     * \brief Recalls previously running Lua scripts stored via <c>store_running_scripts</c> and starts them.
     */
    void load_running_scripts();

    /**
     * \brief Prints text to the console of a Lua context.
     * \param ctx The Lua context to print to.
     * \param text The text to print.
     */
    void print(t_lua_wnd_ctx& ctx, const std::wstring& text);

} // namespace LuaDialog
