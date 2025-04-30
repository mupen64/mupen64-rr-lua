/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * A module responsible for comparison of expected and actual savestates during movie playback. Used for regression testing.
 */
namespace Compare
{
    /**
     * \brief Starts a comparison.
     * \param control Whether the comparison is deemed the control (the correct sequence of savestates).
     * \param interval The comparison interval.
     */
    void start(bool control, size_t interval);

    /**
     * \brief Performs the comparison between the expected and actual savestates at the current sample.
     * \param current_sample The VCR's current sample.
     * \warning Requires corresponding savestates to be present in the saves directory. Note that they are not checked for ROM or movie congruence.
     */
    void compare(size_t current_sample);

    /**
     * \brief Gets whether the comparison system is currently active.
     */
    bool active();

} // namespace Compare
