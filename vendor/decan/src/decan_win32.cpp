
#if defined(_WIN32)
#include "decan.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>

#define NOMINMAX
#include <windows.h>

namespace decan
{

library::library(const std::filesystem::path &file_name) : m_filename(file_name.string())
{
    try
    {
        HMODULE res = LoadLibraryW(file_name.c_str());
        if (res == nullptr)
        {
            DWORD last_error = GetLastError();
            throw std::system_error(last_error, std::system_category());
        }
        m_handle = res;
    }
    catch (...)
    {
        std::throw_with_nested(dll_error("Error at LoadLibraryW()"));
    }
}

library::~library()
{
    if (m_handle != (HMODULE) nullptr) FreeLibrary(m_handle);
}

void *library::get(const char *symbol) const
{
    try
    {
        FARPROC res = GetProcAddress(m_handle, symbol);
        if (res == nullptr)
        {
            DWORD last_error = GetLastError();
            throw std::system_error(last_error, std::system_category());
        }

        return reinterpret_cast<void *>(res);
    }
    catch (...)
    {
        std::throw_with_nested(dll_error("Error at GetProcAddress()"));
    }
}
} // namespace decan

#endif