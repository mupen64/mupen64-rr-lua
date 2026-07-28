/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"

// Shift-JIS bytes, as found in the header name field of Japanese ROMs. These are not valid UTF-8: 0x83 can't start a
// sequence, so converting them to a path with the strict STL conversion throws ERROR_NO_UNICODE_TRANSLATION.
static constexpr char JAPANESE_NAME[] = "\x83\x65\x83\x58\x83\x67";               // テスト
static constexpr char OTHER_JAPANESE_NAME[] = "\x83\x43\x83\x89\x83\x43\x83\x89"; // イライラ

TEST_CASE("ascii_rom_name_is_unchanged", "rom_name_to_path_component")
{
    // Save files on disk are named after what this produced before, so ASCII names have to stay byte-identical.
    REQUIRE(IOUtils::rom_name_to_path_component("SUPER MARIO 64") == std::filesystem::path("SUPER MARIO 64"));
}

TEST_CASE("empty_rom_name_yields_empty_component", "rom_name_to_path_component")
{
    REQUIRE(IOUtils::rom_name_to_path_component("").empty());
}

TEST_CASE("non_utf8_rom_name_does_not_throw", "rom_name_to_path_component")
{
    REQUIRE_NOTHROW(IOUtils::rom_name_to_path_component(JAPANESE_NAME));
    REQUIRE(!IOUtils::rom_name_to_path_component(JAPANESE_NAME).empty());
}

TEST_CASE("distinct_non_utf8_rom_names_stay_distinct", "rom_name_to_path_component")
{
    // Otherwise two Japanese ROMs would share the same save files.
    REQUIRE(IOUtils::rom_name_to_path_component(JAPANESE_NAME) !=
            IOUtils::rom_name_to_path_component(OTHER_JAPANESE_NAME));
}
