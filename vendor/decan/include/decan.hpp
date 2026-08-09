
/**
 * Copyright (C) 2026, Jacky Guo
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef DECAN_HPP
#define DECAN_HPP

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>

// On Windows, applies the stdcall calling convention.
#define DECAN_STDCALL __stdcall
// The platform library extension.
#define DECAN_LIB_EXT ".dll"
#elif defined(__linux__)
// On Windows, applies the stdcall calling convention.
#define DECAN_STDCALL
// The platform library extension.
#define DECAN_LIB_EXT ".so"
#endif

namespace decan
{

#if defined(_WIN32)
using handle_t = HMODULE;
#elif defined(__linux__)
using handle_t = void *;
#endif

class dll_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class library
{
    std::string m_filename;
    handle_t m_handle;

  public:
    // Loads a library from a given path.
    library(const std::filesystem::path &path);

    library(const library &) = delete;
    library &operator=(const library &) = delete;

    library(library &&rhs) : m_filename(std::move(rhs.m_filename)), m_handle(rhs.m_handle)
    {
        // zero the RHS handle to avoid misuse
        rhs.m_handle = (handle_t)0;
    }
    library &operator=(library &&rhs)
    {
        // destruct, then move-construct.
        this->~library();
        new (this) library(std::move(rhs));
        return *this;
    }

    ~library();

    // Gets a symbol from a library.
    // This maps directly to GetProcAddress/dlsym.
    void *get(const char *symbol) const;
};

} // namespace decan

#endif
