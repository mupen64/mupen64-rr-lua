/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>
#include <span>

#include <libdeflate.h>

/**
 * \brief A module providing various miscellaneous helper functions.
 */
namespace MiscHelpers
{
inline void vecwrite(std::vector<uint8_t> &vec, const void *data, const size_t len)
{
    vec.resize(vec.size() + len);
    memcpy(vec.data() + (vec.size() - len), data, len);
}

inline std::vector<uint8_t> auto_decompress(const std::vector<uint8_t> &vec, const size_t initial_size)
{
    if (vec.size() < 2 || vec[0] != 0x1F && vec[1] != 0x8B)
    {
        // vec is decompressed already

        // we need a copy, not ref
        std::vector<uint8_t> out_vec = vec;
        return out_vec;
    }

    // The gzip header does not include the uncompressed size, so grow the output buffer until it fits.
    size_t buf_size = std::max(initial_size, size_t{1});
    auto decompressor = libdeflate_alloc_decompressor();
    if (!decompressor) return {};

    std::vector<uint8_t> out_vec;
    while (true)
    {
        out_vec.resize(buf_size);
        size_t actual_size = 0;
        const auto result = libdeflate_gzip_decompress(decompressor, vec.data(), vec.size(), out_vec.data(),
                                                       out_vec.size(), &actual_size);
        if (result == LIBDEFLATE_SHORT_OUTPUT || result == LIBDEFLATE_INSUFFICIENT_SPACE)
        {
            if (buf_size > std::numeric_limits<size_t>::max() / 2)
            {
                libdeflate_free_decompressor(decompressor);
                return {};
            }
            buf_size *= 2;
            continue;
        }

        libdeflate_free_decompressor(decompressor);
        if (result != LIBDEFLATE_SUCCESS) return {};

        out_vec.resize(actual_size);
        return out_vec;
    }
}

inline std::vector<uint8_t> auto_compress(std::span<const uint8_t> in)
{
    std::vector<uint8_t> compressed;
    compressed.resize(in.size());

    const auto compressor = libdeflate_alloc_compressor(6);
    const size_t final_size =
        libdeflate_gzip_compress(compressor, in.data(), in.size(), compressed.data(), compressed.size());
    libdeflate_free_compressor(compressor);
    compressed.resize(final_size);

    return compressed;
}

inline void memread(uint8_t **src, void *dest, const unsigned int len)
{
    memcpy(dest, *src, len);
    *src += len;
}

inline bool iequals(std::wstring_view lhs, std::wstring_view rhs)
{
    return std::ranges::equal(lhs, rhs,
                              [](const wchar_t a, const wchar_t b) { return std::tolower(a) == std::tolower(b); });
}

inline std::string to_lower(std::string a)
{
    std::ranges::transform(a, a.begin(), [](const unsigned char c) { return std::tolower(c); });
    return a;
}

/**
 * \brief Trims consecutive spaces in a C-style string by replacing the first occurrence of consecutive spaces with a
 * null terminator. \param str The C-style string to trim. \param len The length of the string.
 */
inline void strtrim(char *str, const size_t len)
{
    for (int i = 0; i < len; ++i)
    {
        if (i == 0)
        {
            continue;
        }
        if (str[i - 1] == ' ' && str[i] == ' ')
        {
            memset(str + i - 1, 0, len - i + 1);
            return;
        }
    }
}

/**
 * \brief Erases elements from a vector at specified indices.
 * \tparam T The type of elements in the vector.
 * \param data The original vector from which elements will be erased.
 * \param indices_to_delete A vector of indices indicating which elements to erase.
 * \return A new vector with the specified elements removed.
 */
template <typename T> std::vector<T> erase_indices(const std::vector<T> &data, std::vector<size_t> &indices_to_delete)
{
    if (indices_to_delete.empty()) return data;

    std::vector<T> ret = data;

    std::ranges::sort(indices_to_delete, std::greater<>());
    for (auto i : indices_to_delete)
    {
        if (i >= ret.size())
        {
            continue;
        }
        ret.erase(ret.begin() + i);
    }

    return ret;
}

/**
 * \brief Remaps a value from one range to another.
 * \param value The value to remap.
 * \param from1 The lower bound of the source range.
 * \param to1 The upper bound of the source range.
 * \param from2 The lower bound of the target range.
 * \param to2 The upper bound of the target range.
 * \return The value, remapped to the target range.
 */
template <typename T> inline T remap(const T value, const T from1, const T to1, const T from2, const T to2)
{
    return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}

/**
 * \brief Limits a value to a specific range, wrapping around if it exceeds the bounds.
 * \param value The value to limit.
 * \param min The lower bound.
 * \param max The upper bound.
 * \return The value, limited to the specified range.
 */
template <typename T> inline T wrapping_clamp(const T value, T min, T max)
{
    static_assert(std::is_integral_v<T>, "wrapping_clamp only supports integral types");

    if (min == max)
    {
        return min;
    }

    if (min > max)
    {
        std::swap(min, max);
    }

    const T range = max - min + 1;
    T offset = (value - min) % range;
    if (offset < 0) offset += range;
    return min + offset;
}

/**
 * \brief Limits a value to a specific range, wrapping around if it exceeds the bounds. The wrap around can only happen
 * once.
 * \param value The value to limit.
 * \param min The lower bound.
 * \param max The upper bound.
 * \return The value, limited to the specified range.
 */
template <typename T> static T wrapping_clamp_decimal(T value, T min, T max)
{
    if (value < min)
    {
        return max - (min - value);
    }
    if (value > max)
    {
        return min + (value - max);
    }
    return value;
}
namespace details
{
template <auto Ptr, class F> struct StaticFunctorImpl;

template <auto Ptr, class R, class... Args>
    requires std::is_same_v<decltype(Ptr), R (*)(Args...)>
struct StaticFunctorImpl<Ptr, R (*)(Args...)>
{
    R operator()(Args... args) const { return Ptr(args...); }
};
} // namespace details

/**
 * @brief Wraps an arbitrary (statically-defined) function in a function object, so it can be used as a hash function,
 * deleter, etc.
 *
 * @tparam F A function pointer.
 */
template <auto F> struct StaticFunctor : public details::StaticFunctorImpl<F, decltype(F)>
{
};

}; // namespace MiscHelpers
