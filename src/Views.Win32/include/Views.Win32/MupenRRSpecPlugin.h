/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the new Mupen64-RR Plugin API.
 *
 * This header can be used standalone by Mupen64 plugins, just make sure to define PLUGIN_WITH_CALLBACKS first.
 *
 */

#pragma once

#include "core_types.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#error "Unsupported platform"
#endif

namespace MupenRRSpecPlugin
{
extern "C"
{
};
} // namespace MupenRRSpecPlugin

#if defined(PLUGIN_WITH_CALLBACKS)

extern "C"
{
    // ReSharper disable CppInconsistentNaming

    // ReSharper restore CppInconsistentNaming
}

#endif

// #undef EXPORT
// #undef CALL
