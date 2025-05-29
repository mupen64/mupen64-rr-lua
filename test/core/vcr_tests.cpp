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

#pragma region Unit

// ee2a0e4
TEST(vcr_on_controller_poll, input_callback_called_when_using_input_buffer_during_recording)
{
    prepare_test();

    static bool called = false;
    params.callbacks.input = [](core_buttons* input, int index) {
        called = true;
    };

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
    vcr.current_sample = 2;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    EXPECT_TRUE(called);
}

// 94e3d9d
TEST(read_movie_header, sample_length_gets_clamped_to_buffer_max)
{
    prepare_test();

    core_vcr_movie_header hdr{};
    hdr.magic = 0x1a34364d;
    hdr.version = 3;
    hdr.length_samples = 3;

    std::vector<uint8_t> bytes(sizeof(hdr));
    std::memcpy(bytes.data(), &hdr, sizeof(hdr));
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    bytes.insert(bytes.end(), {0, 0, 0, 0});

    core_vcr_movie_header out_hdr{};
    vcr_read_movie_header(bytes, &out_hdr);

    ASSERT_EQ(out_hdr.length_samples, 2);
}

struct seek_test_params {
    t_vcr_state vcr{};
    std::wstring str{};
    size_t expected_frame{};
};

class SeekTest : public testing::TestWithParam<seek_test_params> {};
TEST_P(SeekTest, seek_stops_at_expected_frame)
{
    prepare_test();

    const auto param = GetParam();
    vcr = param.vcr;

    bool seek_completed = false;
    params.callbacks.seek_completed = [&] {
        seek_completed = true;
    };

    core_init(&params);

    const auto result = core_vcr_begin_seek(param.str, false);
    ASSERT_EQ(result, Res_Ok);

    while (!seek_completed)
    {
        core_buttons input{};
        vcr_on_controller_poll(0, &input);
    }

    ASSERT_EQ(param.expected_frame + 1, vcr.current_sample);
}
INSTANTIATE_TEST_CASE_P(
seek_tests,
SeekTest,
::testing::Values(seek_test_params{
                  .vcr = {
                  .task = task_playback,
                  .hdr = {
                  .length_samples = 5,
                  .controller_flags = CONTROLLER_X_PRESENT(0),
                  },
                  .inputs = {
                  core_buttons{0x01},
                  core_buttons{0x02},
                  core_buttons{0x03},
                  core_buttons{0x04},
                  core_buttons{0x05},
                  },
                  .current_sample = 0,
                  },
                  .str = L"3",
                  .expected_frame = 3,
                  },

                  seek_test_params{
                  .vcr = {
                  .task = task_playback,
                  .hdr = {
                  .length_samples = 5,
                  .controller_flags = CONTROLLER_X_PRESENT(0),
                  },
                  .inputs = {
                  core_buttons{0x01},
                  core_buttons{0x02},
                  core_buttons{0x03},
                  core_buttons{0x04},
                  core_buttons{0x05},
                  },
                  .current_sample = 3,
                  },
                  .str = L"-1",
                  .expected_frame = 2,
                  }));


#pragma endregion
