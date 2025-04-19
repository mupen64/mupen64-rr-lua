/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing frontend components for the CoreDbg feature.
 */
namespace CoreDbg
{
    /**
     * Initializes the subsystem.
     */
    void init();

    /**
     * \brief Shows the CoreDbg dialog
     */
    void show();
} // namespace CoreDbg
