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

} // namespace Validators