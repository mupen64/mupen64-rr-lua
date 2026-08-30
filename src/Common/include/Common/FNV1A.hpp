/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>
#include <span>

/**
 * \brief A module providing a FNV-1a hash function implementation.
 */
namespace FNV1A
{
constexpr uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr uint64_t fnv_prime = 1099511628211ULL;

inline uint64_t hash(std::span<const uint8_t> data, uint64_t hash = fnv_offset_basis)
{
    const auto *bytes = static_cast<const uint8_t *>(data.data());
    for (size_t i = 0; i < data.size(); ++i)
    {
        hash ^= bytes[i];
        hash *= fnv_prime;
    }
    return hash;
}

} // namespace FNV1A
