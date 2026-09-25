/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "Validators.hpp"

namespace Validators
{

std::optional<std::string> int32_t(const std::string_view str)
{
    try
    {
        std::size_t pos;
        std::ignore = std::stoi(std::string(str), &pos);
        if (pos != str.size())
        {
            return "Value must be an integer.";
        }
    }
    catch (const std::exception &)
    {
        return "Value must be an integer.";
    }
    return std::nullopt;
}

std::optional<std::string> int32_t_optional(const std::string_view str)
{
    if (str.empty())
    {
        return std::nullopt;
    }
    return int32_t(str);
}

std::optional<std::string> boolean(const std::string_view str)
{
    if (str == "0" || str == "1")
    {
        return std::nullopt;
    }
    return "Value must be either 0 or 1.";
}

std::optional<std::string> none(const std::string_view)
{
    return std::nullopt;
}

std::optional<std::string> nonempty(const std::string_view str)
{
    return str.empty() ? std::make_optional("Value must not be empty.") : std::nullopt;
}

std::optional<std::string> seek_str(const std::string_view str)
{
    const auto result = g_main_ctx.CoreCtx->vcr_try_resolve_seek_str(std::string(str));
    return result.has_value() ? std::nullopt : std::make_optional("Invalid seek string.");
}

std::optional<std::string> existing_path(const std::string_view str)
{
    return std::filesystem::exists(str) ? std::nullopt : std::make_optional("Path does not exist.");
}

std::optional<std::string> rom_path(const std::string_view str)
{
    std::filesystem::path path(str);
    if (!std::filesystem::exists(path))
    {
        return std::make_optional("ROM path does not exist.");
    }

    const auto ext = path.extension();

    for (const auto &valid_ext : VALID_ROM_EXTENSIONS)
    {
        if (ext == std::format(".{}", valid_ext))
        {
            return std::nullopt;
        }
    }

    return std::make_optional("Invalid ROM file extension.");
}

std::optional<std::string> lua_path(const std::string_view str)
{
    std::filesystem::path path(str);
    if (!std::filesystem::exists(path))
    {
        return std::make_optional("Lua script path does not exist.");
    }

    const auto ext = path.extension();

    for (const auto &valid_ext : VALID_LUA_EXTENSIONS)
    {
        if (ext == std::format(".{}", valid_ext))
        {
            return std::nullopt;
        }
    }

    return std::make_optional("Invalid Lua script file extension.");
}

} // namespace Validators
