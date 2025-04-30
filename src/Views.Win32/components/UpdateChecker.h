/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * A module responsible for online update checking.
 */
namespace UpdateChecker
{
    /**
     * Checks for updates.
     * @param manual Whether the update check was initiated via a deliberate user interaction.
     */
    void check(bool manual);
}; // namespace UpdateChecker
