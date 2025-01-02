#pragma once

#include <Windows.h>
#include <filesystem>
#include <shlobj_core.h>
#include <string>
#include <vector>

/**
 * \brief Gets all files under all subdirectory of a specific directory,
 * including the directory's shallow files \param directory The path
 * joiner-terminated directory
 */
static std::vector<std::wstring> get_files_in_subdirectories(std::wstring directory)
{
    if (directory.back() != L'\\')
    {
        directory += L"\\";
    }
    WIN32_FIND_DATA find_file_data;
    const HANDLE h_find = FindFirstFile((directory + L"*").c_str(), &find_file_data);
    if (h_find == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    std::vector<std::wstring> paths;
    std::wstring fixed_path = directory;
    do
    {
        if (!lstrcmpW(find_file_data.cFileName, L".") || !lstrcmpW(find_file_data.cFileName, L".."))
            continue;

        auto full_path = directory + find_file_data.cFileName;

        if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            paths.push_back(full_path);
            continue;
        }

        if (directory[directory.size() - 2] == '\0')
        {
            if (directory.back() == '\\')
            {
                fixed_path.pop_back();
                fixed_path.pop_back();
            }
        }

        if (directory.back() != '\\')
        {
            fixed_path.push_back('\\');
        }

        full_path = fixed_path + find_file_data.cFileName;
        for (const auto& path : get_files_in_subdirectories(full_path + L"\\"))
        {
            paths.push_back(path);
        }
    }
    while (FindNextFile(h_find, &find_file_data) != 0);

    FindClose(h_find);

    return paths;
}

/**
 * \brief Removes the extension from a path
 * \param path The path to remove the extension from
 * \return The path without an extension
 */
static std::string strip_extension(const std::string& path)
{
    size_t i = path.find_last_of('.');

    if (i != std::string::npos)
    {
        return path.substr(0, i);
    }
    return path;
}

/**
 * \brief Removes the extension from a path
 * \param path The path to remove the extension from
 * \return The path without an extension
 */
static std::wstring strip_extension(const std::wstring& path)
{
    size_t i = path.find_last_of('.');

    if (i != std::string::npos)
    {
        return path.substr(0, i);
    }
    return path;
}

/**
 * \brief Gets the path to the current user's desktop
 */
static std::wstring get_desktop_path()
{
    wchar_t path[MAX_PATH + 1] = {0};
    SHGetSpecialFolderPathW(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);
    return path;
}

/**
 * \brief Creates a new path with a different name, keeping the extension and
 * preceeding path intact \param path A path \param name The new name \return
 * The modified path
 */
static std::filesystem::path with_name(std::filesystem::path path, std::string name)
{
    char drive[MAX_PATH] = {0};
    char dir[MAX_PATH] = {0};
    char filename[MAX_PATH] = {0};
    _splitpath(path.string().c_str(), drive, dir, filename, nullptr);

    return std::filesystem::path(std::string(drive) + std::string(dir) + name + path.extension().string());
}

/**
 * \brief Gets the name (filename without extension) of a path
 * \param path A path
 * \return The path's name
 */
static std::string get_name(std::filesystem::path path)
{
    char filename[MAX_PATH] = {0};
    _splitpath(path.string().c_str(), nullptr, nullptr, filename, nullptr);
    return filename;
}
