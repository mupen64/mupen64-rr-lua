/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#elif defined(__linux__)
#include <stdio.h>
#endif

namespace IOUtils
{

// FILE UTILITIES
// ==============================

// reads a file from beginning to end.
inline std::vector<uint8_t> read_entire_file(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return {};
    }

    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        return {};
    }

    return buffer;
}

// overwrites the contents of a file with the provided buffer.
inline bool write_entire_file(const std::filesystem::path &path, std::span<uint8_t> data)
{
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open())
    {
        return false;
    }

    out.write(reinterpret_cast<const char *>(data.data()), data.size());
    return out.good();
}

// Checks if two files are equal. Returns 0 if not equal, 1 if equal, and -1 on error.
inline int file_contents_equal(const std::filesystem::path &path1, const std::filesystem::path &path2)
{
    constexpr size_t CHUNK_SIZE = 4096;

    std::ifstream file1(path1, std::ios::in | std::ios::binary);
    std::ifstream file2(path2, std::ios::in | std::ios::binary);

    if (file1.fail() || file2.fail())
        return -1;

    // compare file sizes using seekg()
    file1.seekg(0, std::ios::end);
    file2.seekg(0, std::ios::end);

    if (file1.tellg() != file2.tellg())
        return 0;

    file1.seekg(0, std::ios::end);
    file2.seekg(0, std::ios::end);

    // files are same length, read char-by-char until we find something not equal.
    // normally this isn't efficient, but because C++ handles the buffering for us
    // it's no big deal.
    while (!file1.eof() && !file2.eof()) {
        int c1 = file1.get();
        int c2 = file2.get();
        if (c1 == std::char_traits<char>::eof() || c2 == std::char_traits<char>::eof()) {
            return -1;
        }
        if (c1 != c2) {
            return 0;
        }
    }
    return 1;
}

#ifdef _WIN32
// Selects the platform-native path type for strings and characters.
// This is a minor optimization for Windows which avoids the char -> wchar_t conversion at runtime.
#define MUPEN64_PATH_T(s) L##s
#else
// Selects the platform-native path type for strings and characters.
// This is a minor optimization for Windows which avoids the char -> wchar_t conversion at runtime.
#define MUPEN64_PATH_T(s) s
#endif

// IOSTREAM UTILITIES
// ==============================

template <class IStreamT, class CharT = typename IStreamT::char_type, class Traits = typename IStreamT::traits_type>
    requires(std::derived_from<IStreamT, std::basic_istream<CharT, Traits>>)
inline auto iter_lines(IStreamT &stream);

namespace details
{
class IOLineSentinel
{
};

template <class IStreamT, class CharT = typename IStreamT::char_type, class Traits = typename IStreamT::traits_type>
    requires(std::derived_from<IStreamT, std::basic_istream<CharT, Traits>>)
class IOLineIterator
{
  private:
    using istream_type = IStreamT;
    using char_type = CharT;
    using traits_type = Traits;

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::basic_string_view<CharT, Traits>;
    using reference_type = std::basic_string_view<CharT, Traits>;

    template <class IStreamT2, class CharT2, class Traits2>
        requires(std::derived_from<IStreamT2, std::basic_istream<CharT2, Traits2>>)
    friend inline auto ::IOUtils::iter_lines(IStreamT2 &stream);

    value_type operator*() const { return m_line; }

    IOLineIterator &operator++()
    {
        assert(!m_stream->fail());
        std::getline(*m_stream, m_line);
        return *this;
    }

    void operator++(int)
    {
        // we can post-increment, but there is no way to return a nice value
        // so just return nothing
        ++(*this);
    }

    friend bool operator==(const IOLineIterator &iter, IOLineSentinel) { return iter.m_stream->fail(); }

    istream_type &istream() { return m_stream; }

  private:
    IOLineIterator(istream_type &stream) : m_stream(&stream), m_line()
    {
        // This class operates under the assumption that m_stream is never null.
        assert(m_stream != nullptr);
        assert(!m_stream->fail());
        std::getline(*m_stream, m_line);
    }

    istream_type *m_stream;
    std::basic_string<CharT, Traits> m_line;
};

static_assert(std::input_iterator<IOLineIterator<std::ifstream>>);
} // namespace details

// Returns an iterator over the lines of text in an input stream.
template <class IStreamT, class CharT, class Traits>
    requires(std::derived_from<IStreamT, std::basic_istream<CharT, Traits>>)
inline auto iter_lines(IStreamT &stream)
{
    return std::ranges::subrange{details::IOLineIterator<IStreamT, CharT, Traits>(stream), details::IOLineSentinel()};
}

// WINDOWS UTF-16 CONVERSION
// ==============================

#ifdef _WIN32

inline std::wstring to_wide_string(std::string_view str)
{
    using namespace std::string_literals;

    assert(str.size() < INT_MAX / 2); // sanity check

    if (str.empty())
    {
        return L""s;
    }

    // return code
    int rc;

    // figure out how much space we need
    rc = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), str.size(), nullptr, 0);
    if (rc == 0)
    {
        // throw std::system_error(rc, std::system_category(), "invalid UTF-8");
        return L""s;
    }

    // This is the only safe way to do it, it's a bit of a shame there's no way to turn an arbitrary allocation
    // into a vector/string/whatever
    std::wstring output;
    output.resize(static_cast<size_t>(rc), L'\0');

    rc = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), str.size(), output.data(), output.size());
    if (rc == 0)
    {
        // throw std::system_error(rc, std::system_category(), "failed UTF-8 -> UTF-16 conversion");
        return L""s;
    }

    return output;
}

inline std::string to_utf8_string(std::wstring_view wstr)
{
    using namespace std::string_literals;

    assert(wstr.size() < INT_MAX / 2); // sanity check

    if (wstr.empty())
    {
        return ""s;
    }

    // return code
    int rc;

    // figure out how much space we need
    rc = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(), wstr.size(), nullptr, 0, 0, nullptr);
    if (rc == 0)
    {
        // throw std::system_error(rc, std::system_category(), "invalid UTF-16");
        return ""s;
    }

    // This is the only safe way to do it, it's a bit of a shame there's no way to turn an arbitrary allocation
    // into a vector/string/whatever
    std::string output;
    output.resize(static_cast<size_t>(rc), '\0');

    rc = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(), wstr.size(), output.data(), output.size(), 0,
                             nullptr);
    if (rc == 0)
    {
        // throw std::system_error(rc, std::system_category(), "invalid UTF-16");
        return ""s;
    }

    return output;
}

#endif

// PORTABLE EQUIVALENTS
// ==============================

// Portable version of fopen_s using std::filesystem::path.
inline int path_fopen_s(FILE *&stream, const std::filesystem::path &path, const char *mode)
{
#ifdef _WIN32
    auto mode_wc = to_wide_string(mode);
    return _wfopen_s(&stream, path.c_str(), mode_wc.c_str());
#else
    FILE* ptr = fopen(path.c_str(), mode);
    if (ptr == nullptr)
        return errno;

    stream = ptr;
    return 0;
#endif
}

// Portable version of Windows `_wfsopen(path, mode, _SH_DENYNO)`.
inline FILE *path_fopen_shared(const std::filesystem::path &path, const char *mode)
{
#ifdef _WIN32
    auto mode_wc = to_wide_string(mode);
    return _wfsopen(path.c_str(), mode_wc.c_str(), _SH_DENYNO);
#else
    // Linux file locks are opt-in.
    return fopen(path.c_str(), mode);
#endif
}

// Gets the path of the current executable file.
inline std::filesystem::path exe_path()
{
#ifdef _WIN32
    wchar_t path_buffer[MAX_PATH] = {L'\0'};
    int rc;

    rc = GetModuleFileNameW(NULL, path_buffer, sizeof(path_buffer) / sizeof(wchar_t));
    if (rc == 0)
    {
        throw std::system_error(GetLastError(), std::system_category());
    }
    return std::filesystem::path(path_buffer);
#endif
}
// Gets the path of the current executable file.
inline std::filesystem::path exe_path_cached()
{
    // this ensures that the exe path is cached.
    static std::filesystem::path cached_path = exe_path();
    return cached_path;
}

} // namespace IOUtils