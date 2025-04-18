/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * A module responsible for benchmarking end-to-end performance.
 */
namespace Benchmark
{
    typedef struct {
        double fps;
    } t_result;

    /**
     * \brief Starts a benchmark.
     */
    void start();

    /**
     * \brief Stops the benchmark. Writes the result to the provided struct.
     */
    void stop(t_result*);

    /**
     * \brief Saves the benchmark result to a file.
     * \param path The path to the file.
     * \param result The result to save.
     */
    void save_result_to_file(const std::filesystem::path& path, const t_result& result);

    /**
     * \brief Notifies about a new frame.
     */
    void frame();
} // namespace Benchmark
