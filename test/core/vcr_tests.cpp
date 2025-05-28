/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdafx.h>
#include <Core/r4300/vcr.h>
#include <gtest/gtest.h>

extern t_vcr_state vcr;
static core_cfg cfg{};
static core_params params{};

#pragma region Integration

static void prepare_test()
{
    vcr = {};
    cfg = {};
    params.cfg = &cfg;
    params.plugin_funcs.input_get_keys = [](int32_t controller, core_buttons* keys) {
    };
    params.plugin_funcs.input_set_keys = [](int32_t controller, core_buttons keys) {
    };
}

TEST(vcr_on_controller_poll, reset_pending_returns_unmodified_input)
{
    prepare_test();

    const auto INPUT_VALUE = 0xDEAD;

    core_init(&params);

    vcr.reset_pending = true;

    core_buttons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, INPUT_VALUE);
}

TEST(vcr_on_controller_poll, seek_savestate_loading_returns_unmodified_input)
{
    prepare_test();

    const auto INPUT_VALUE = 0xDEAD;

    core_init(&params);

    vcr.seek_savestate_loading = true;

    core_buttons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, INPUT_VALUE);
}

TEST(vcr_on_controller_poll, idle_task_returns_input_from_getkeys)
{
    prepare_test();

    const auto INPUT_VALUE = 0xDEAD;

    params.plugin_funcs.input_get_keys = [](int32_t index, core_buttons* input) {
        *input = {INPUT_VALUE};
    };
    core_init(&params);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, INPUT_VALUE);
}

TEST(vcr_on_controller_poll, playback_returns_correct_input)
{
    prepare_test();
    
    core_init(&params);

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 2;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(input.value, inputs[2].value);
}

TEST(vcr_on_controller_poll, record_appends_input)
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

    core_init(&params);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 4;

    core_buttons input{0xDEAD};
    vcr_on_controller_poll(0, &input);

    EXPECT_EQ(vcr.inputs.back().value, 0xDEAD);
}


TEST(vcr_on_controller_poll, seek_continues_when_end_not_reached)
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{
        {1},
        {2},
        {3},
        {4}};

    core_init(&params);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 1;
    vcr.seek_to_frame = std::make_optional(3);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    EXPECT_TRUE(vcr.seek_to_frame.has_value());
}

TEST(vcr_on_controller_poll, seek_stops_when_end_reached)
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

    core_init(&params);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 3;
    vcr.seek_to_frame = std::make_optional(3);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    EXPECT_FALSE(vcr.seek_to_frame.has_value());
}

#pragma endregion
