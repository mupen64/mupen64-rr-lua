/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <json.hpp>
#include <features/Benchmark.h>

static size_t frames{};
static std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

void Benchmark::start()
{
    start_time = std::chrono::high_resolution_clock::now();
}

void Benchmark::stop(t_result* result)
{
    const auto now = std::chrono::high_resolution_clock::now();
    result->fps = (double)frames / ((double)std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time).count() / 1000000000.0);
}

void Benchmark::save_result_to_file(const std::filesystem::path& path, const t_result& result)
{
    nlohmann::json j;
    j["fps"] = result.fps;

    std::ofstream of(path);
    of << j.dump(4);
    of.close();
}

void Benchmark::frame()
{
    frames++;
}
