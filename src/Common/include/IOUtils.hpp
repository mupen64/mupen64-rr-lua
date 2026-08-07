/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>

#include "StrUtils.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <share.h>
#elif defined(__linux__)
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iconv.h>
#else
#error Unsupported platform!
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

    if (file1.fail() || file2.fail()) return -1;

    // compare file sizes using seekg()
    file1.seekg(0, std::ios::end);
    file2.seekg(0, std::ios::end);

    if (file1.tellg() != file2.tellg()) return 0;

    file1.seekg(0, std::ios::end);
    file2.seekg(0, std::ios::end);

    // files are same length, read char-by-char until we find something not equal.
    // normally this isn't efficient, but because C++ handles the buffering for us
    // it's no big deal.
    while (!file1.eof() && !file2.eof())
    {
        int c1 = file1.get();
        int c2 = file2.get();
        if (c1 == std::char_traits<char>::eof() || c2 == std::char_traits<char>::eof())
        {
            return -1;
        }
        if (c1 != c2)
        {
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

/**
 * @brief Converts UTF-8 to UTF-16.
 *
 * @param str
 * @return std::wstring
 */
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

/**
 * \brief Decodes a raw ROM header name into a wide string.
 * \param str Pointer to the start of the 20-byte ROM header.
 *
 * The N64 SDK specifies the header name field as JIS X 0201 / Shift-JIS. This function *will* error if the ROM header
 * is not valid Shift-JIS and may cause undefined behaviour if less than 20 bytes are available through `str`.
 */
inline std::wstring rom_name_to_wide_string(const char str[20])
{
    using namespace std::string_literals;

    // Windows-932 isn't *exactly* Shift-JIS, but it's close enough that it shouldn't matter.
    constexpr UINT CP_SHIFT_JIS = 932;

    // return code
    int rc;

    // figure out how much space we need
    rc = MultiByteToWideChar(CP_SHIFT_JIS, 0, str, 20, nullptr, 0);
    if (rc == 0)
    {
        // throw std::system_error(rc, std::system_category(), "invalid UTF-8");
        return L""s;
    }

    // This is the only safe way to do it, it's a bit of a shame there's no way to turn an arbitrary allocation
    // into a vector/string/whatever
    std::wstring output;
    output.resize(static_cast<size_t>(rc), L'\0');

    rc = MultiByteToWideChar(CP_SHIFT_JIS, 0, str, 20, output.data(), output.size());
    if (rc == 0)
    {
        // throw std::system_error(rc, std::system_category(), "failed UTF-8 -> UTF-16 conversion");
        return L""s;
    }

    // Trim off trailing nulls
    if (size_t first_null = output.find_first_of(L'\0'); first_null != std::string::npos)
    {
        output.resize(first_null);
    }
    // Trim to remove spaces at the end; ROM headers are typically padded to 20 characters with spaces.
    return std::wstring{StrUtils::ctrim_wstring(output)};
}

#endif

// SHIFT-JIS DECODING via iconv.h
// ====================================

#ifdef __linux__

/**
 * \brief Decodes a raw ROM header name into a wide string.
 * \param str Pointer to the start of the 20-byte ROM header.
 *
 * The N64 SDK specifies the header name field as JIS X 0201 / Shift-JIS. This function *will* error if the ROM header
 * is not valid Shift-JIS and may cause undefined behaviour if less than 20 bytes are available through `str`.
 */
inline std::string rom_name_to_utf8(const char str[20])
{
    using namespace std::literals;

    class IConvHandle
    {
      public:
        IConvHandle(iconv_t conv) : m_inner(conv) {}

        ~IConvHandle() { iconv_close(m_inner); }

        operator iconv_t() { return m_inner; }

      private:
        iconv_t m_inner;
    };

    IConvHandle converter = iconv_open("UTF-8", "SHIFT-JIS");

    // iconv is not guaranteed to preserve the input buffer, so copy it out first
    char src_buf[20] = {0};
    memcpy(src_buf, str, 20);

    // setup source pointers for conversion
    char *src_ptr = src_buf;
    size_t src_len = 20;

    // The Shift-JIS string contains at most 20 characters. If all are katakana, they take up at most 3 bytes per
    // character as they are in the BMP. It can be shown that inserting a 2-byte character can only reduce the length of
    // the encoding from 60.
    std::string dst_buffer(60, '\0');
    char *dst_ptr = dst_buffer.data();
    size_t dst_len = dst_buffer.size();

    // Perform the conversion. This should complete in one step.
    size_t result = iconv(converter, &src_ptr, &src_len, &dst_ptr, &dst_len);
    if (result == -1)
    {
        throw std::system_error(errno, std::generic_category(), "iconv() failed");
        // return ""s;
    }

    // reduce buffer from 60 chars to the correct size
    size_t true_length = dst_ptr - dst_buffer.data();
    dst_buffer.resize(true_length);

    // trim off trailing nulls
    if (size_t first_null = dst_buffer.find_first_of('\0'); first_null != std::string::npos)
    {
        dst_buffer.resize(first_null);
    }

    // Trim whitespace that was added in the ROM.
    return std::string{StrUtils::ctrim_string(dst_buffer)};
}
#endif

// ROM HEADER NAME CONVERSION
// ==============================

/**
 * \brief Converts a raw ROM header name into a path component.
 * \param str Pointer to the start of the 20-byte ROM header.
 *
 * The N64 SDK specifies the header name field as JIS X 0201 / Shift-JIS. This function *will* error if the ROM header
 * is not valid Shift-JIS and may cause undefined behaviour if less than 20 bytes are available through `str`.
 */
inline std::filesystem::path rom_name_to_path_component(const char str[20])
{
#ifdef _WIN32
    return {rom_name_to_wide_string(str)};
#else
    return {rom_name_to_utf8(str)};
#endif
}

// PORTABLE EQUIVALENTS
// ==============================

// Portable version of fopen_s using std::filesystem::path.
inline int path_fopen_s(FILE *&stream, const std::filesystem::path &path, const char *mode)
{
#ifdef _WIN32
    auto mode_wc = to_wide_string(mode);
    return _wfopen_s(&stream, path.c_str(), mode_wc.c_str());
#else
    FILE *ptr = fopen(path.c_str(), mode);
    if (ptr == nullptr) return errno;

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

// Computes the path of the current executable file.
inline std::filesystem::path compute_exe_path()
{
#ifdef _WIN32
    wchar_t path_buffer[MAX_PATH] = {L'\0'};
    DWORD rc;

    rc = GetModuleFileNameW(NULL, path_buffer, sizeof(path_buffer) / sizeof(wchar_t));
    if (rc == 0)
    {
        throw std::system_error((int)GetLastError(), std::system_category());
    }
    return std::filesystem::path(path_buffer);
#elif defined(__linux__)
    return std::filesystem::read_symlink("/proc/self/exe");
#else
#error TODO: compute_exe_path() not defined on this platform
#endif
}

// Gets the path of the current executable file.
// This is only computed once and cached for the rest of the program.
inline const std::filesystem::path &exe_path()
{
    // this ensures that the exe path is cached.
    static const std::filesystem::path cached_path = compute_exe_path();
    return cached_path;
}

// Computes the path of the config directory.
inline std::filesystem::path compute_config_path()
{
#ifdef _WIN32
    wchar_t path_buffer[MAX_PATH] = {L'\0'};
    DWORD rc;
    rc = GetEnvironmentVariableW(L"LOCALAPPDATA", path_buffer, MAX_PATH);
    if (rc == 0)
    {
        throw std::system_error((int)GetLastError(), std::system_category());
    }

    auto dir = std::filesystem::path(path_buffer) / "mupen64-rr-lua";
    return dir;
#elif defined(__linux__)
    const char *env_config = getenv("XDG_CONFIG_HOME");
    if (env_config != nullptr)
    {
        return std::filesystem::path(env_config) / "mupen64-rr-lua";
    }

    const char *env_home = getenv("HOME");
    if (env_home == nullptr) throw std::runtime_error("$HOME is undefined");

    return std::filesystem::path(env_home) / ".config/mupen64-rr-lua";
#else
#error TODO: compute_config_dir not defined on this platform
#endif
}

/**
 * @brief Gets the path to the config directory.
 *
 * This is usually tied to `%LOCALAPPDATA%` on Windows, and `$XDG_CONFIG_HOME` or `~/.config`
 * on Linux.
 */
inline const std::filesystem::path &config_path()
{
    static const std::filesystem::path cached_path = compute_config_path();
    if (!std::filesystem::is_directory(cached_path)) std::filesystem::create_directories(cached_path);
    return cached_path;
}

} // namespace IOUtils
