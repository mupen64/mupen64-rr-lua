/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Validators.h"
#include "stdafx.h"

namespace Validators
{

std::optional<std::wstring> int32_t(const std::wstring_view str)
{
    try
    {
        std::size_t pos;
        std::stoi(str.data(), &pos);
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

std::optional<std::wstring> none(const std::wstring_view)
{
    return std::nullopt;
}

std::optional<std::wstring> nonempty(const std::wstring_view str)
{
    return str.empty() ? std::make_optional(L"Value must not be empty.") : std::nullopt;
}
} // namespace Validators