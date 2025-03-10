/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Writes data to a vector at its tail end based on its size, expanding it as needed
 * \param vec The target vector
 * \param data The source data
 * \param len The source data's size in bytes
 */
void vecwrite(std::vector<uint8_t>& vec, void* data, size_t len);

/**
 * \brief Reads a file into a buffer
 * \param path The file's path
 * \return The file's contents, or an empty vector if the operation failed
 */
std::vector<uint8_t> read_file_buffer(const std::filesystem::path& path);

/**
 * \brief Writes a buffer to a file
 * \param path The file's path
 * \param data The buffer
 * \return Whether the operation succeeded
 */
bool write_file_buffer(const std::filesystem::path& path, std::span<uint8_t> data);

/**
 * \brief Decompresses an (optionally) gzip-compressed byte vector
 * \param vec The byte vector
 * \param initial_size The initial size to allocate for the internal buffer
 * \return The decompressed byte vector
 */
std::vector<uint8_t> auto_decompress(std::vector<uint8_t>& vec, size_t initial_size = 0xB624F0);

/**
 * \brief Reads source data into the destination, advancing the source pointer by <c>len</c>
 * \param src A pointer to the source data
 * \param dest The destination buffer
 * \param len The destination buffer's length
 */
void memread(uint8_t** src, void* dest, unsigned int len);

bool ichar_equals(wchar_t a, wchar_t b);

bool iequals(std::wstring_view lhs, std::wstring_view rhs);

std::string to_lower(std::string a);

bool contains(const std::string& a, const std::string& b);

/**
 * Erases the specified indicies in a vector.
 * \tparam T The vector's contained type.
 * \param data The vector.
 * \param indices_to_delete The indices to delete.
 * \return The vector with the indicies removed.
 */
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
 * Converts a string to a wstring. Returns an empty string on fail.
 * \param str The string to convert
 * \return The string's wstring representation
 */
std::wstring string_to_wstring(const std::string& str);

/**
 * Converts a wstring to a string.
 * \param wstr The wstring to convert
 * \return The wstring's string representation
 */
std::string wstring_to_string(const std::wstring& wstr);

/**
 * Splits a string by a delimiter
 * \param s The string to split
 * \param delimiter The delimiter
 * \return A vector of string segments split by the delimiter
 * \remark See https://stackoverflow.com/a/46931770/14472122
 */
std::vector<std::wstring> split_string(const std::wstring& s, const std::wstring& delimiter);

// FIXME: Use template...

/**
 * Splits a wstring by a delimiter
 * \param s The wstring to split
 * \param delimiter The delimiter
 * \return A vector of wstring segments split by the delimiter
 * \remark See https://stackoverflow.com/a/46931770/14472122
 */
std::vector<std::wstring> split_wstring(const std::wstring& s, const std::wstring& delimiter);

/**
 * Writes a nul terminator at the last space in the string
 * \param str The string to trim
 * \param len The string's length including the nul terminator
 */
void strtrim(char* str, size_t len);

/**
 * Finds the nth occurence of a pattern inside a string
 * \param str The string to search in
 * \param searched The pattern to search for
 * \param nth The occurence index
 * \return The start index of the occurence into the string, or std::string::npos if none was found
 */
size_t str_nth_occurence(const std::string& str, const std::string& searched, size_t nth);

/**
 * \brief Checks whether files are byte-wise equal. If the files are not found, the function returns false. If the files are not of equal length, the function returns false.
 * \param first Path to the first file.
 * \param second Path to the second file.
 * \remarks The files are opened in binary mode, hence "byte-wise".
 */
bool files_are_equal(const std::filesystem::path& first, const std::filesystem::path& second);

/**
  * \brief Gets all files with a specific file extension directly under a directory.
  * \param directory The directory.
  * \param extension The file extension with no period.
  */
std::vector<std::wstring> get_files_with_extension_in_directory(std::wstring directory, const std::wstring& extension);
