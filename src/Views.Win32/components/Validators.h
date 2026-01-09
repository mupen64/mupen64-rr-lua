/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing common action parameter validators.
 */
namespace Validators
{

std::optional<std::wstring> int32_t(const std::wstring_view str);

std::optional<std::wstring> int32_t_optional(const std::wstring_view str);

std::optional<std::wstring> none(const std::wstring_view);

std::optional<std::wstring> nonempty(const std::wstring_view str);

std::optional<std::wstring> seek_str(const std::wstring_view str);

} // namespace Validators