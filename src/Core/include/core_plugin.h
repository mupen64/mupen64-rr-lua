/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Describes the Mupen64 core-side Plugin API.
 *
 * This header can be used standalone by Mupen64 plugins.
 *
 */

#pragma once

#include "core_types.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __cdecl
#else
#define EXPORT
#define CALL
#endif

extern "C"
{
    /**
     * \brief Describes a controller.
     */
    typedef struct
    {
        int32_t Present;
        int32_t RawData;
        int32_t Plugin;
    } core_controller;

    /**
     * \brief Represents an extension for a controller.
     */
    typedef enum
    {
        ce_none = 1,
        ce_mempak = 2,
        ce_rumblepak = 3,
        ce_transferpak = 4,
        ce_raw = 5
    } core_controller_extension;

    /**
     * \brief Describes framebuffer information.
     */
    typedef struct
    {
        uint32_t addr;
        uint32_t size;
        uint32_t width;
        uint32_t height;
    } core_fb_info;

    /**
     * \brief Represents a controller state.
     */
    typedef union {
        uint32_t value;

        struct
        {
            unsigned dr : 1;
            unsigned dl : 1;
            unsigned dd : 1;
            unsigned du : 1;
            unsigned start : 1;
            unsigned z : 1;
            unsigned b : 1;
            unsigned a : 1;
            unsigned cr : 1;
            unsigned cl : 1;
            unsigned cd : 1;
            unsigned cu : 1;
            unsigned r : 1;
            unsigned l : 1;
            unsigned reserved_1 : 1;
            unsigned reserved_2 : 1;
            signed x : 8;
            signed y : 8;
        };
    } core_buttons;
}

inline bool operator==(const core_buttons &lhs, const core_buttons &rhs)
{
    return lhs.value == rhs.value;
}

#undef EXPORT
#undef CALL
