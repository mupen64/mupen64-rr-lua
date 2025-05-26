/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdafx.h>
#include <Core/r4300/vcr.h>
#include <gtest/gtest.h>

extern t_vcr_state vcr;

#pragma region Integration

static void reset_vcr_state()
{
    vcr = {};
}

TEST(vcr_on_controller_poll, reset_pending_returns_unmodified_input)
{
    reset_vcr_state();

    const auto INPUT_VALUE = 0xDEAD;

    core_params params{};
    core_init(&params);

    vcr.reset_pending = true;

    core_buttons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, INPUT_VALUE);
}

TEST(vcr_on_controller_poll, seek_savestate_loading_returns_unmodified_input)
{
    reset_vcr_state();

    const auto INPUT_VALUE = 0xDEAD;

    core_params params{};
    core_init(&params);

    vcr.seek_savestate_loading = true;

    core_buttons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, INPUT_VALUE);
}


TEST(vcr_on_controller_poll, idle_task_returns_input_from_getkeys)
{
    reset_vcr_state();

    const auto INPUT_VALUE = 0xDEAD;

    core_params params{};
    params.plugin_funcs.input_get_keys = [](int32_t index, core_buttons* input) {
        *input = {INPUT_VALUE};
    };
    core_init(&params);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, INPUT_VALUE);
}

#pragma endregion
