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

const std::vector<std::wstring> VALID_ROM_EXTENSIONS = {L"n64", L"z64", L"v64", L"rom", L"bin",
                                                        L"zip", L"usa", L"eur", L"jap"};
const std::vector<std::wstring> VALID_LUA_EXTENSIONS = {L"lua"};

/**
 * \brief Validates whether the input string represents a valid 32-bit integer.
 * \param str The input string to validate.
 * \return An optional error message if validation fails, or std::nullopt if validation succeeds.
 */
std::optional<std::wstring> int32_t(const std::wstring_view str);

/**
 * \brief Validates whether the input string represents a valid 32-bit integer or is empty.
 * \param str The input string to validate.
 * \return An optional error message if validation fails, or std::nullopt if validation succeeds or the string is empty.
 */
std::optional<std::wstring> int32_t_optional(const std::wstring_view str);

/**
 * \brief Validates whether the input string represents a boolean value (0 or 1).
 * \param str The input string to validate.
 * \return An optional error message if validation fails, or std::nullopt if validation succeeds or the string is empty.
 */
std::optional<std::wstring> boolean(const std::wstring_view str);

/**
 * \brief A validator that always returns no error.
 * \param str The input string to validate.
 * \return Always returns std::nullopt.
 */
std::optional<std::wstring> none(const std::wstring_view str);

/**
 * \brief Validates whether the input string is non-empty.
 * \param str The input string to validate.
 * \return An optional error message if the string is empty, or std::nullopt if it is non-empty.
 */
std::optional<std::wstring> nonempty(const std::wstring_view str);

/**
 * \brief Validates whether the input string is a valid seek string.
 * \param str The input string to validate.
 * \return An optional error message if validation fails, or std::nullopt if validation succeeds.
 */
std::optional<std::wstring> seek_str(const std::wstring_view str);

/**
 * \brief Validates whether the input string represents an existing filesystem path.
 * \param str The input string to validate.
 * \return An optional error message if the path does not exist, or std::nullopt if it exists.
 */
std::optional<std::wstring> existing_path(const std::wstring_view str);

/**
 * \brief Validates whether the input string represents a valid ROM path.
 * \param str The input string to validate.
 * \return An optional error message if the path does not exist, or std::nullopt if it exists.
 */
std::optional<std::wstring> rom_path(const std::wstring_view str);

/**
 * \brief Validates whether the input string represents a valid Lua script path.
 * \param str The input string to validate.
 * \return An optional error message if the path does not exist, or std::nullopt if it exists.
 */
std::optional<std::wstring> lua_path(const std::wstring_view str);

} // namespace Validators