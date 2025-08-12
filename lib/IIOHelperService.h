/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <libdeflate.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <stringapiset.h>
#endif

class IIOHelperService {
public:
    virtual ~IIOHelperService() = default;

    virtual void vecwrite(std::vector<uint8_t>& vec, void* data, size_t len)
    {
        vec.resize(vec.size() + len);
        memcpy(vec.data() + (vec.size() - len), data, len);
    }

    virtual std::vector<uint8_t> read_file_buffer(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            return {};
        }

        const auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        {
            return {};
        }

        return buffer;
    }

    virtual bool write_file_buffer(const std::filesystem::path& path, std::span<uint8_t> data)
    {
        std::ofstream out(path, std::ios::binary);
        if (!out)
        {
            return false;
        }

        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        return out.good();
    }

    virtual std::vector<uint8_t> auto_decompress(std::vector<uint8_t>& vec, size_t initial_size)
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

    virtual void memread(uint8_t** src, void* dest, unsigned int len)
    {
        memcpy(dest, *src, len);
        *src += len;
    }

    virtual bool iequals(std::wstring_view lhs, std::wstring_view rhs)
    {
        return std::ranges::equal(lhs, rhs, [](wchar_t a, wchar_t b) {
            return std::tolower(a) == std::tolower(b);
        });
    }

    virtual std::string to_lower(std::string a)
    {
        std::ranges::transform(a, a.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return a;
    }

    virtual bool contains(const std::string& a, const std::string& b)
    {
        return a.find(b) != std::string::npos;
    }

    virtual std::wstring string_to_wstring(const std::string& str)
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

    virtual std::string wstring_to_string(const std::wstring& wstr)
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

    virtual std::vector<std::wstring> split_string(const std::wstring& s, const std::wstring& delimiter)
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

    virtual std::vector<std::wstring> split_wstring(const std::wstring& s, const std::wstring& delimiter)
    {
        std::vector<std::wstring> res;
        res.reserve(4);

        if (delimiter.empty())
        {
            res.push_back(s);
            return res;
        }

        size_t pos_start = 0;
        size_t pos_end;
        const size_t delim_len = delimiter.length();

        while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos)
        {
            res.emplace_back(s, pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
        }

        res.emplace_back(s, pos_start, s.size() - pos_start);

        return res;
    }

    virtual void strtrim(char* str, size_t len)
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
    virtual std::wstring ltrim(const std::wstring& str)
    {
        std::wstring s = str;
        s.erase(s.begin(), std::ranges::find_if(s, [](const wchar_t ch) {
                    return !iswspace(ch);
                }));
        return s;
    }

    /**
     * \brief Removes trailing whitespace from a string.
     * \param str The string to trim.
     * \return A new string with trailing whitespace removed.
     */
    virtual std::wstring rtrim(const std::wstring& str)
    {
        std::wstring s = str;
        s.erase(std::find_if(s.rbegin(), s.rend(), [](const wchar_t ch) {
                    return !iswspace(ch);
                })
                .base(),
                s.end());
        return s;
    }

    /**
     * \brief Removes leading and trailing whitespace from a string.
     * \param str The string to trim.
     * \return A new string with leading and trailing whitespace removed.
     */
    virtual std::wstring trim(const std::wstring& str)
    {
        if (str.empty())
            return {};

        size_t start = 0;
        size_t end = str.size();

        while (start < end && iswspace(str[start]))
            ++start;

        while (end > start && iswspace(str[end - 1]))
            --end;

        return str.substr(start, end - start);
    }

    virtual size_t str_nth_occurence(const std::wstring& str, const std::wstring& searched, size_t nth)
    {
        if (searched.empty() || nth <= 0)
        {
            return std::wstring::npos;
        }

        size_t pos = 0;
        int count = 0;

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

    virtual bool files_are_equal(const std::filesystem::path& first, const std::filesystem::path& second)
    {
        bool different = false;

        FILE* fp1 = nullptr;
        FILE* fp2 = nullptr;
        if (_wfopen_s(&fp1, first.wstring().c_str(), L"rb") || _wfopen_s(&fp2, second.wstring().c_str(), L"rb"))
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


    virtual std::vector<std::wstring> get_files_with_extension_in_directory(std::wstring directory, const std::wstring& extension)
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

    template <typename T>
    std::vector<T> erase_indices(const std::vector<T>& data, std::vector<size_t>& indices_to_delete)
    {
        if (indices_to_delete.empty())
            return data;

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
     * \brief Joins a vector of strings into a single string with a specified delimiter.
     * \param vec The vector of strings to join.
     * \param delimiter The delimiter to use between the strings.
     * \return A single string containing all elements of the vector separated by the delimiter.
     */
    virtual std::wstring join_wstring(const std::vector<std::wstring>& vec, const std::wstring& delimiter)
    {
        std::wostringstream s;
        for (const auto& i : vec)
        {
            if (&i != &vec[0])
            {
                s << delimiter;
            }
            s << i;
        }
        return s.str();
    }

    struct t_path_segment_info {
        std::wstring drive{};
        std::wstring dir{};
        std::wstring filename{};
        std::wstring ext{};
    };

    /**
     * \brief Gets information about a path's segments.
     * \param path The path to analyze.
     * \param info The structure to fill with path segment information.
     * \return Whether the operation succeeded.
     */
    virtual bool get_path_segment_info(const std::filesystem::path& path, t_path_segment_info& info)
    {
        info.drive = std::wstring(_MAX_DRIVE, 0);
        info.dir = std::wstring(_MAX_DIR, 0);
        info.filename = std::wstring(_MAX_FNAME, 0);
        info.ext = std::wstring(_MAX_EXT, 0);

        const auto result = !(bool)_wsplitpath_s(path.wstring().c_str(), info.drive.data(), info.drive.size(), info.dir.data(), info.dir.size(), info.filename.data(), info.filename.size(), info.ext.data(), info.ext.size());

        if (!result)
        {
            return false;
        }

        auto trim_str = [](std::wstring& str) {
            const size_t null_pos = str.find(L'\0');
            if (null_pos != std::wstring::npos && null_pos > 0)
            {
                str.resize(null_pos);
            }
            else if (null_pos == 0)
            {
                str.clear();
            }
        };

        trim_str(info.drive);
        trim_str(info.dir);
        trim_str(info.filename);
        trim_str(info.ext);

        return true;
    }
};
