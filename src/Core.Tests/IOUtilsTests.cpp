/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"

// Selection of 20-byte ROM headers.
static constexpr char MARIO64[20] = "SUPER MARIO 64";
static constexpr char EMPTY[20] = "";
static constexpr char NORMAL_JAPANESE[20] =
    "\x8e\x84\x82\xcd\x83\x65\x83\x58\x83\x67\x82\xbe\x82\xe6\x0a"; // 私はテストだよ  (I am a test)
static constexpr char HALF_WIDTH_CHARS[20] =
    "\xca\xdd\xb6\xb8\xb6\xc0\xb6\xc5\xc3\xbd\xc4\xc0\xde\xd6\x0a"; // ﾊﾝｶｸｶﾀｶﾅﾃｽﾄﾀﾞﾖ (This is a half-width katakana
                                                                    // test)

TEST_CASE("ascii_rom_name_is_unchanged", "rom_name_to_path_component")
{
    // Save files on disk are named after what this produced before, so ASCII names have to stay byte-identical.
    REQUIRE(IOUtils::rom_name_to_path_component(MARIO64) == std::filesystem::path(MARIO64));
}

TEST_CASE("empty_rom_name_yields_empty_component", "rom_name_to_path_component")
{
    REQUIRE(IOUtils::rom_name_to_path_component(EMPTY).empty());
}

TEST_CASE("non_utf8_rom_name_does_not_throw", "rom_name_to_path_component")
{
    REQUIRE_NOTHROW(IOUtils::rom_name_to_path_component(NORMAL_JAPANESE));
    REQUIRE(!IOUtils::rom_name_to_path_component(NORMAL_JAPANESE).empty());
}

TEST_CASE("distinct_non_utf8_rom_names_stay_distinct", "rom_name_to_path_component")
{
    // Otherwise two Japanese ROMs would share the same save files.
    REQUIRE(IOUtils::rom_name_to_path_component(NORMAL_JAPANESE) !=
            IOUtils::rom_name_to_path_component(HALF_WIDTH_CHARS));
}
