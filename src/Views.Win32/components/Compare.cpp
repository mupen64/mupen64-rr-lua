/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Compare.h"

static uint8_t compare_mode = 0;
static size_t compare_interval = 0;

void Compare::start(bool control, size_t interval)
{
    compare_mode = control ? 1 : 2;
    compare_interval = interval;
}

void Compare::compare(size_t current_sample)
{
    if (compare_mode == 0)
    {
        return;
    }

    if (current_sample % compare_interval != 0)
    {
        return;
    }

    const auto save_dir = Config::save_directory();

    if (compare_mode == 2)
    {
        if (current_sample > compare_interval * 2)
        {
            const auto expected_path = save_dir / std::format(L"cmp_expected_{}.st", current_sample - compare_interval);
            const auto actual_path = save_dir / std::format(L"cmp_actual_{}.st", current_sample - compare_interval);

            if (IOUtils::file_contents_equal(expected_path, actual_path) == 1)
            {
                g_view_logger->info("MATCH at frame {}", current_sample - compare_interval);
            }
            else
            {
                g_view_logger->error("DIFFERENCE at frame {}", current_sample - compare_interval);
            }
        }

        const auto actual_path = save_dir / std::format(L"cmp_actual_{}.st", current_sample);
        g_main_ctx.core_ctx->st_do_file(actual_path.c_str(), core_st_job_save, nullptr, true);

        return;
    }

    const auto path = save_dir / std::format(L"cmp_expected_{}.st", current_sample);
    g_main_ctx.core_ctx->st_do_file(path.c_str(), core_st_job_save, nullptr, true);
}

bool Compare::active()
{
    return compare_mode;
}
