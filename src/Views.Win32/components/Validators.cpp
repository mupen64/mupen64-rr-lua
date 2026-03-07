/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Validators.h"
#include "stdafx.h"

namespace Validators
{

std::optional<std::wstring> int32_t(const std::wstring_view str)
{
    try
    {
        std::size_t pos;
        std::ignore = std::stoi(std::wstring(str), &pos);
        if (pos != str.size())
        {
            return L"Value must be an integer.";
        }
    }
    catch (const std::exception &)
    {
        return L"Value must be an integer.";
    }
    return std::nullopt;
}

std::optional<std::wstring> int32_t_optional(const std::wstring_view str)
{
    if (str.empty())
    {
        return std::nullopt;
    }
    return int32_t(str);
}

std::optional<std::wstring> boolean(const std::wstring_view str)
{
    if(str == L"0" || str == L"1")
    {
        return std::nullopt;
    }
    return L"Value must be either 0 or 1.";
}

std::optional<std::wstring> none(const std::wstring_view)
{
    return std::nullopt;
}

std::optional<std::wstring> nonempty(const std::wstring_view str)
{
    return str.empty() ? std::make_optional(L"Value must not be empty.") : std::nullopt;
}

std::optional<std::wstring> seek_str(const std::wstring_view str)
{
    const auto utf8_str = IOUtils::to_utf8_string(std::wstring(str));
    const auto result = g_main_ctx.core_ctx->vcr_try_resolve_seek_str(utf8_str);
    return result.has_value() ? std::nullopt : std::make_optional(L"Invalid seek string.");
}

std::optional<std::wstring> existing_path(const std::wstring_view str)
{
    return std::filesystem::exists(str) ? std::nullopt : std::make_optional(L"Path does not exist.");
}

std::optional<std::wstring> rom_path(const std::wstring_view str)
{
    std::filesystem::path path(str);
    if (!std::filesystem::exists(path))
    {
        return std::make_optional(L"ROM path does not exist.");
    }

    const auto ext = path.extension().wstring();

    for (const auto &valid_ext : VALID_ROM_EXTENSIONS)
    {
        if (ext == std::format(L".{}", valid_ext))
        {
            return std::nullopt;
        }
    }

    return std::make_optional(L"Invalid ROM file extension.");
}

std::optional<std::wstring> lua_path(const std::wstring_view str)
{
    std::filesystem::path path(str);
    if (!std::filesystem::exists(path))
    {
        return std::make_optional(L"Lua script path does not exist.");
    }

    const auto ext = path.extension().wstring();

    for (const auto &valid_ext : VALID_LUA_EXTENSIONS)
    {
        if (ext == std::format(L".{}", valid_ext))
        {
            return std::nullopt;
        }
    }

    return std::make_optional(L"Invalid Lua script file extension.");
}

} // namespace Validators