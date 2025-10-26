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

// Case-insensitive comparison of C strings.
inline int c_nicmp(const char* a, const char* b, size_t n) {
#if defined(_WIN32)
    return _strnicmp(a, b, n);
#elif defined (__unix__)  || (defined(__APPLE__) && defined(__MACH__))
    return strncasecmp(a, b, n);
#else
    #error unknown operating system!
#endif
    
}

} // namespace StrUtils