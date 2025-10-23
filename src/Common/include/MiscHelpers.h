/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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

    // we dont know what the decompressed size is, so we reallocate a buffer until we find the right size lol
    size_t buf_size = initial_size;
    auto out_buf = static_cast<uint8_t *>(malloc(buf_size));
    auto decompressor = libdeflate_alloc_decompressor();
    while (true)
    {
        out_buf = static_cast<uint8_t *>(realloc(out_buf, buf_size));
        size_t actual_size = 0;
        auto result = libdeflate_gzip_decompress(decompressor, vec.data(), vec.size(), out_buf, buf_size, &actual_size);
        if (result == LIBDEFLATE_SHORT_OUTPUT || result == LIBDEFLATE_INSUFFICIENT_SPACE)
        {
            buf_size *= 2;
            continue;
        }
        buf_size = actual_size;
        break;
    }
    libdeflate_free_decompressor(decompressor);

    out_buf = static_cast<uint8_t *>(realloc(out_buf, buf_size));
    std::vector<uint8_t> out_vec;
    out_vec.resize(buf_size);
    memcpy(out_vec.data(), out_buf, buf_size);
    free(out_buf);
    return out_vec;
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
 * \brief Checks if string 'a' contains string 'b'.
 * \param a
 * \param b
 * \return
 */
inline bool contains(const std::string &a, const std::string &b)
{
    return a.find(b) != std::string::npos;
}

/**
 * \brief Splits a wide string into a vector of wide strings based on a specified delimiter.
 * \param str The wide string to split.
 * \param delimiter The delimiter to split the string by.
 * \return A vector of wide strings resulting from the split operation.
 */
inline std::vector<std::wstring> split_wstring(const std::wstring &str, const std::wstring &delimiter)
{
    std::vector<std::wstring> res;
    res.reserve(4);

    if (delimiter.empty())
    {
        res.push_back(str);
        return res;
    }

    size_t pos_start = 0;
    size_t pos_end;
    const size_t delim_len = delimiter.length();

    while ((pos_end = str.find(delimiter, pos_start)) != std::wstring::npos)
    {
        res.emplace_back(str, pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
    }

    res.emplace_back(str, pos_start, str.size() - pos_start);

    return res;
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
 * \brief Removes leading whitespace from a string.
 * \param str The string to trim.
 * \return A new string with leading whitespace removed.
 */
inline std::wstring ltrim(const std::wstring &str)
{
    std::wstring s = str;
    s.erase(s.begin(), std::ranges::find_if(s, [](const wchar_t ch) { return !iswspace(ch); }));
    return s;
}

/**
 * \brief Removes trailing whitespace from a string.
 * \param str The string to trim.
 * \return A new string with trailing whitespace removed.
 */
inline std::wstring rtrim(const std::wstring &str)
{
    std::wstring s = str;
    s.erase(std::find_if(s.rbegin(), s.rend(), [](const wchar_t ch) { return !iswspace(ch); }).base(), s.end());
    return s;
}

/**
 * \brief Removes leading and trailing whitespace from a string.
 * \param str The string to trim.
 * \return A new string with leading and trailing whitespace removed.
 */
inline std::wstring trim(const std::wstring &str)
{
    if (str.empty()) return {};

    size_t start = 0;
    size_t end = str.size();

    while (start < end && iswspace(str[start])) ++start;

    while (end > start && iswspace(str[end - 1])) --end;

    return str.substr(start, end - start);
}

/**
 * \brief Finds the position of the nth occurrence of a substring within a string.
 * \param str The string to search within.
 * \param searched The substring to search for.
 * \param nth The occurrence number to find (1-based).
 * \return The position of the nth occurrence, or std::wstring::npos if not found.
 */
inline size_t str_nth_occurence(const std::wstring &str, const std::wstring &searched, const size_t nth)
{
    if (searched.empty() || nth <= 0)
    {
        return std::wstring::npos;
    }

    size_t pos = 0;
    size_t count = 0;

    while (count < nth)
    {
        pos = str.find(searched, pos);
        if (pos == std::wstring::npos)
        {
            return std::wstring::npos;
        }
        count++;
        if (count < nth)
        {
            pos += searched.size();
        }
    }

    return pos;
}

/**
 * \brief Joins a vector of strings into a single string with a specified delimiter.
 * \param vec The vector of strings to join.
 * \param delimiter The delimiter to use between the strings.
 * \return A single string containing all elements of the vector separated by the delimiter.
 */
inline std::wstring join_wstring(const std::vector<std::wstring> &vec, const std::wstring &delimiter)
{
    std::wostringstream s;
    for (const auto &i : vec)
    {
        if (&i != vec.data())
        {
            s << delimiter;
        }
        s << i;
    }
    return s.str();
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

// Returns an iterator to split `str` by `delim`.
template <class CharT, class Traits = std::char_traits<CharT>>
inline auto split_basic_string(std::basic_string_view<CharT, Traits> str, std::basic_string_view<CharT, Traits> delim);

namespace details
{
// Sentinel for StringSplitIterator.
class StringSplitSentinel
{
};

// Iterator over the parts of a string, split by a delimiter.
template <class CharT, class Traits = std::char_traits<CharT>> class StringSplitIterator
{
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::basic_string_view<CharT, Traits>;

    template <class CharT2, class Traits2>
    inline friend auto ::MiscHelpers::split_basic_string(std::basic_string_view<CharT2, Traits2> str,
                                   std::basic_string_view<CharT2, Traits2> delim);

    value_type operator*() const
    {
        assert(m_start != value_type::npos);
        return m_str.substr(m_start, (m_end == value_type::npos) ? m_end : m_end - m_start);
    }

    StringSplitIterator &operator++()
    {
        assert(m_start != value_type::npos);
        if (m_end == value_type::npos)
        {
            m_start = value_type::npos;
        }
        else
        {
            m_start = m_end + 1;
            m_end = m_str.find(m_delim, m_start);
        }
        return *this;
    }
    StringSplitIterator operator++(int)
    {
        StringSplitIterator temp = *this;
        ++(*this);
        return temp;
    }

    friend bool operator==(const StringSplitIterator &iter, StringSplitSentinel)
    {
        return iter.m_start == value_type::npos;
    }

  private:
    StringSplitIterator() = delete;

    StringSplitIterator(std::basic_string_view<CharT, Traits> str, std::basic_string_view<CharT, Traits> delim)
        : m_str(str), m_delim(delim), m_start(0), m_end(str.find(delim))
    {
    }

    std::basic_string_view<CharT, Traits> m_str;
    std::basic_string_view<CharT, Traits> m_delim;

    size_t m_start;
    size_t m_end;
};

static_assert(std::input_iterator<StringSplitIterator<char>>);
} // namespace details

// Returns an iterator to split `str` by `delim`.
template <class CharT, class Traits>
inline auto split_basic_string(std::basic_string_view<CharT, Traits> str, std::basic_string_view<CharT, Traits> delim)
{
    return std::ranges::subrange(details::StringSplitIterator(str, delim), details::StringSplitSentinel{});
}

// Returns an iterator to split `str` by `delim`.
inline auto split_string(std::string_view str, std::string_view delim)
{
    return split_basic_string<char>(str, delim);
}

// Returns an iterator to split `str` by `delim`.
inline auto split_wstring(std::wstring_view str, std::wstring_view delim)
{
    return split_basic_string<wchar_t>(str, delim);
}

}; // namespace MiscHelpers
