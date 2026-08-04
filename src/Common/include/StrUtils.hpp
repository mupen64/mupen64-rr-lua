/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string_view>
#include <string>

namespace StrUtils
{

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
    inline friend auto ::StrUtils::split_basic_string(std::basic_string_view<CharT2, Traits2> str,
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

// CASE-INSENSITIVE COMPARISON
// ==============================

// Case-insensitive comparison of C strings.
inline int c_icmp(const char *a, const char *b)
{
#if defined(_WIN32)
    return _stricmp(a, b);
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    return strcasecmp(a, b);
#else
#error unknown operating system!
#endif
}

// Case-insensitive comparison of C strings. (with a length limit)
inline int c_nicmp(const char *a, const char *b, size_t n)
{
#if defined(_WIN32)
    return _strnicmp(a, b, n);
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    return strncasecmp(a, b, n);
#else
#error unknown operating system!
#endif
}

// Trim
// ==============================

// Trims whitespace from the start and end of a string_view (as determined by isspace()).
inline std::string_view ctrim_string(std::string_view str)
{
    using namespace std::literals;

    const auto not_isspace = [](wchar_t c) { return !isspace(c); };

    // search from the start for non-whitespace
    auto start_iter = std::ranges::find_if(str, not_isspace);
    if (start_iter == str.end()) return ""sv;
    auto str2 = str.substr(start_iter - str.begin());

    auto end_iter = std::ranges::find_if(std::views::reverse(str2), not_isspace);
    if (end_iter == str2.rend()) return ""sv;
    return str2.substr(0, end_iter.base() - str2.begin());
}
// Trims whitespace from the start and end of a wstring_view (as determined by iswspace()).
inline std::wstring_view ctrim_wstring(std::wstring_view str)
{
    using namespace std::literals;

    const auto not_iswspace = [](wchar_t c) { return !iswspace(c); };

    // search from the start for non-whitespace
    auto start_iter = std::ranges::find_if(str, not_iswspace);
    if (start_iter == str.end()) return L""sv;
    auto str2 = str.substr(start_iter - str.begin());

    auto end_iter = std::ranges::find_if(std::views::reverse(str2), not_iswspace);
    if (end_iter == str2.rend()) return L""sv;
    return str2.substr(0, end_iter.base() - str2.begin());
}

/**
 * \brief Joins a vector of strings into a single string with a specified delimiter.
 * \param vec The vector of strings to join.
 * \param delimiter The delimiter to use between the strings.
 * \return A single string containing all elements of the vector separated by the delimiter.
 */
inline std::wstring join_wstring(const std::vector<std::wstring> &vec, std::wstring_view delimiter)
{
    std::wostringstream s;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) s << delimiter;
        s << *it;
    }
    return s.str();
}

// STRING-KEYED MAPS
// ==============================

// Heterogeneous hash function for strings and string-like types.
template <class CharT, class Traits = std::char_traits<CharT>> struct StringHash
{
    using is_transparent = void;

    size_t operator()(std::basic_string_view<CharT, Traits> str) const
    {
        return std::hash<std::basic_string_view<CharT, Traits>>{}(str);
    }

    size_t operator()(const CharT *str) const { return std::hash<std::basic_string_view<CharT, Traits>>{}(str); }

    size_t operator()(const std::basic_string<CharT, Traits> &str) const
    {
        return std::hash<std::basic_string<CharT, Traits>>{}(str);
    }
};

// map using std::string as the key that can perform efficient lookup with other string types.
template <class V> using unordered_string_map = std::unordered_map<std::string, V, StringHash<char>, std::equal_to<>>;

// map using std::wstring as the key that can perform efficient lookup with other string types.
template <class V>
using unordered_wstring_map = std::unordered_map<std::wstring, V, StringHash<wchar_t>, std::equal_to<>>;

} // namespace StrUtils