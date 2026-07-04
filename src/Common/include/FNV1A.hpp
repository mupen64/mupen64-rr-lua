/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module providing a FNV-1a hash function implementation.
 */
namespace FNV1A
{
constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

inline uint64_t hash(std::span<const uint8_t> data, uint64_t hash = FNV_OFFSET_BASIS)
{
    const auto *bytes = static_cast<const uint8_t *>(data.data());
    for (size_t i = 0; i < data.size(); ++i)
    {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

} // namespace FNV1A
