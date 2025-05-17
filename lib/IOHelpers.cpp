/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "IOHelpers.h"
#include <libdeflate.h>

#ifdef _WIN32
#include <Windows.h>
#include <stringapiset.h>
#endif

void vecwrite(std::vector<uint8_t>& vec, void* data, size_t len)
{
    vec.resize(vec.size() + len);
    memcpy(vec.data() + (vec.size() - len), data, len);
}

std::vector<uint8_t> read_file_buffer(const std::filesystem::path& path)
{
    FILE* f = nullptr;

    if (fopen_s(&f, path.string().c_str(), "rb"))
    {
        return {};
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> b;
    b.resize(len);

    fread(b.data(), sizeof(uint8_t), len, f);

    fclose(f);
    return b;
}

bool write_file_buffer(const std::filesystem::path& path, std::span<uint8_t> data)
{
    FILE* f = nullptr;

    if (fopen_s(&f, path.string().c_str(), "wb"))
    {
        return false;
    }

    fwrite(data.data(), sizeof(uint8_t), data.size(), f);
    fclose(f);

    return true;
}

std::vector<uint8_t> auto_decompress(std::vector<uint8_t>& vec, size_t initial_size)
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
    auto out_buf = static_cast<uint8_t*>(malloc(buf_size));
    auto decompressor = libdeflate_alloc_decompressor();
    while (true)
    {
        out_buf = static_cast<uint8_t*>(realloc(out_buf, buf_size));
        size_t actual_size = 0;
        auto result = libdeflate_gzip_decompress(
        decompressor,
        vec.data(),
        vec.size(),
        out_buf,
        buf_size,
        &actual_size);
        if (result == LIBDEFLATE_SHORT_OUTPUT || result == LIBDEFLATE_INSUFFICIENT_SPACE)
        {
            buf_size *= 2;
            continue;
        }
        buf_size = actual_size;
        break;
    }
    libdeflate_free_decompressor(decompressor);

    out_buf = static_cast<uint8_t*>(realloc(out_buf, buf_size));
    std::vector<uint8_t> out_vec;
    out_vec.resize(buf_size);
    memcpy(out_vec.data(), out_buf, buf_size);
    free(out_buf);
    return out_vec;
}

void memread(uint8_t** src, void* dest, unsigned int len)
{
    memcpy(dest, *src, len);
    *src += len;
}

bool ichar_equals(wchar_t a, wchar_t b)
{
    return std::tolower(a) == std::tolower(b);
}

bool iequals(std::wstring_view lhs, std::wstring_view rhs)
{
    return std::ranges::equal(lhs, rhs, ichar_equals);
}

std::string to_lower(std::string a)
{
    std::ranges::transform(a, a.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return a;
}

bool contains(const std::string& a, const std::string& b)
{
    return a.find(b) != std::string::npos;
}

std::wstring string_to_wstring(const std::string& str)
{
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size_needed <= 0)
        return L"";

    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    wstr.pop_back();
    return wstr;
#endif
}

std::string wstring_to_string(const std::wstring& wstr)
{
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
        return "";

    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
    str.pop_back();
    return str;
#endif
}

std::vector<std::wstring> split_string(const std::wstring& s, const std::wstring& delimiter)
{
    size_t pos_start = 0, pos_end;
    const size_t delim_len = delimiter.length();
    std::vector<std::wstring> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos)
    {
        auto token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.emplace_back(s.substr(pos_start));
    return res;
}

// FIXME: Use template...

std::vector<std::wstring> split_wstring(const std::wstring& s, const std::wstring& delimiter)
{
    size_t pos_start = 0, pos_end;
    const size_t delim_len = delimiter.length();
    std::vector<std::wstring> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos)
    {
        std::wstring token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.emplace_back(s.substr(pos_start));
    return res;
}

void strtrim(char* str, size_t len)
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

size_t str_nth_occurence(const std::string& str, const std::string& searched, size_t nth)
{
    if (searched.empty() || nth <= 0)
    {
        return std::string::npos;
    }

    size_t pos = 0;
    int count = 0;

    while (count < nth)
    {
        pos = str.find(searched, pos);
        if (pos == std::string::npos)
        {
            return std::string::npos;
        }
        count++;
        if (count < nth)
        {
            pos += searched.size();
        }
    }

    return pos;
}

bool files_are_equal(const std::filesystem::path& first, const std::filesystem::path& second)
{
    bool different = false;

    FILE* fp1 = nullptr;
    FILE* fp2 = nullptr;
    if (fopen_s(&fp1, first.string().c_str(), "rb") || fopen_s(&fp2, second.string().c_str(), "rb"))
    {
        return false;
    }
    fseek(fp1, 0, SEEK_END);
    fseek(fp2, 0, SEEK_END);

    const auto len1 = ftell(fp1);
    const auto len2 = ftell(fp2);

    if (len1 != len2)
    {
        different = true;
        goto cleanup;
    }

    fseek(fp1, 0, SEEK_SET);
    fseek(fp2, 0, SEEK_SET);

    int ch1, ch2;
    while ((ch1 = fgetc(fp1)) != EOF && (ch2 = fgetc(fp2)) != EOF)
    {
        if (ch1 != ch2)
        {
            different = true;
            break;
        }
    }

cleanup:
    fclose(fp1);
    fclose(fp2);
    return !different;
}


std::vector<std::wstring> get_files_with_extension_in_directory(std::wstring directory, const std::wstring& extension)
{
#ifdef _WIN32
    if (directory.empty())
    {
        directory = L".\\";
    }
    else
    {
        if (directory.back() != L'\\')
        {
            directory += L"\\";
        }
    }

    WIN32_FIND_DATA find_file_data;
    const HANDLE h_find = FindFirstFile((directory + L"*." + extension).c_str(), &find_file_data);
    if (h_find == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    std::vector<std::wstring> paths;

    do
    {
        if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            paths.emplace_back(directory + find_file_data.cFileName);
        }
    }
    while (FindNextFile(h_find, &find_file_data) != 0);

    FindClose(h_find);

    return paths;
#endif
}
