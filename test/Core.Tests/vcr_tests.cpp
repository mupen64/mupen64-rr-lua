/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core/r4300/vcr.h>
#include <Core/r4300/r4300.h>

static core_cfg cfg{};
static core_params params{};
static core_ctx *ctx = nullptr;
// static PlatformService io_helper_service{};

/**
 * \brief Initializes the test environment by resetting the vcr state and core parameters, as well as filling out some
 * required functions.
 */
static void prepare_test()
{
    vcr = {};
    cfg = {};
    params.cfg = &cfg;
    // params.io_service = &io_helper_service;
    params.input_get_keys = [](int32_t, core_buttons *) {};
    params.input_set_keys = [](int32_t, core_buttons) {};
}

/**
 * \brief Checks whether the VCR lock is held by trying to grab it from a separate thread.
 */
static bool is_vcr_lock_held()
{
    bool unlocked;
    std::thread([&] {
        unlocked = vcr_mtx.try_lock();
        if (unlocked)
        {
            vcr_mtx.unlock();
        }
    }).join();
    return !unlocked;
}

#pragma region Integration

TEST_CASE("reset_pending_returns_unmodified_input", "vcr_on_controller_poll")
{
    prepare_test();

    const auto INPUT_VALUE = 0xDEAD;

    core_create(&params, &ctx);

    vcr.reset_pending = true;

    core_buttons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE("seek_savestate_loading_returns_unmodified_input", "vcr_on_controller_poll")
{
    prepare_test();

    const auto INPUT_VALUE = 0xDEAD;

    core_create(&params, &ctx);

    vcr.seek_savestate_loading = true;

    core_buttons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE("idle_task_returns_input_from_getkeys", "vcr_on_controller_poll")
{
    prepare_test();

    const auto INPUT_VALUE = 0xDEAD;

    params.input_get_keys = [](int32_t index, core_buttons *input) { *input = {INPUT_VALUE}; };
    core_create(&params, &ctx);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE("playback_returns_correct_input", "vcr_on_controller_poll")
{
    prepare_test();

    core_create(&params, &ctx);

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 2;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == inputs[2].value);
}

/*
 * Tests that vcr_on_controller_poll returns the correct input for multiple controllers during playback.
 */
TEST_CASE("playback_returns_correct_input_2", "vcr_on_controller_poll")
{
    prepare_test();

    core_create(&params, &ctx);

    const auto inputs = std::vector<core_buttons>{
        {0}, {0}, {1}, {1}, {2}, {2},
    };

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0) | CONTROLLER_X_PRESENT(1);
    vcr.task = task_playback;
    vcr.current_sample = 0;

    core_buttons input{};

    vcr_on_controller_poll(0, &input);
    REQUIRE(input.value == inputs[0].value);

    vcr_on_controller_poll(1, &input);
    REQUIRE(input.value == inputs[1].value);

    vcr_on_controller_poll(0, &input);
    REQUIRE(input.value == inputs[2].value);

    vcr_on_controller_poll(1, &input);
    REQUIRE(input.value == inputs[3].value);

    vcr_on_controller_poll(0, &input);
    REQUIRE(input.value == inputs[4].value);

    vcr_on_controller_poll(1, &input);
    REQUIRE(input.value == inputs[5].value);
}

/*
 * Tests that vcr_on_controller_poll returns the correct input for multiple sparse controllers during playback.
 */
TEST_CASE("playback_returns_correct_input_3", "vcr_on_controller_poll")
{
    prepare_test();

    core_create(&params, &ctx);

    const auto inputs = std::vector<core_buttons>{
        {0}, {0}, {0}, {1}, {1}, {1}, {2}, {2}, {2},
    };

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0) | CONTROLLER_X_PRESENT(2) | CONTROLLER_X_PRESENT(3);
    vcr.task = task_playback;
    vcr.current_sample = 0;

    core_buttons input{};

    vcr_on_controller_poll(0, &input);
    REQUIRE(input.value == inputs[0].value);

    vcr_on_controller_poll(2, &input);
    REQUIRE(input.value == inputs[1].value);

    vcr_on_controller_poll(3, &input);
    REQUIRE(input.value == inputs[2].value);

    vcr_on_controller_poll(0, &input);
    REQUIRE(input.value == inputs[3].value);

    vcr_on_controller_poll(2, &input);
    REQUIRE(input.value == inputs[4].value);

    vcr_on_controller_poll(3, &input);
    REQUIRE(input.value == inputs[5].value);

    vcr_on_controller_poll(0, &input);
    REQUIRE(input.value == inputs[6].value);

    vcr_on_controller_poll(2, &input);
    REQUIRE(input.value == inputs[7].value);

    vcr_on_controller_poll(3, &input);
    REQUIRE(input.value == inputs[8].value);
}

TEST_CASE("record_appends_input", "vcr_on_controller_poll")
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 4;

    core_buttons input{0xDEAD};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.inputs.back().value == 0xDEAD);
}

TEST_CASE("seek_continues_when_end_not_reached", "vcr_on_controller_poll")
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 1;
    vcr.seek_to_frame = std::make_optional(3);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.seek_to_frame.has_value());
}

TEST_CASE("seek_stops_when_end_reached", "vcr_on_controller_poll")
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 3;
    vcr.seek_to_frame = std::make_optional(3);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(!vcr.seek_to_frame.has_value());
}

#pragma endregion

#pragma region Unit

// ee2a0e4
TEST_CASE("input_callback_called_when_using_input_buffer_during_recording", "vcr_on_controller_poll")
{
    prepare_test();

    static bool called = false;
    params.callbacks.input = [](core_buttons *input, int index) { called = true; };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 2;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

// 94e3d9d
TEST_CASE("sample_length_gets_clamped_to_buffer_max", "read_movie_header")
{
    prepare_test();

    core_vcr_movie_header hdr{};
    hdr.magic = 0x1a34364d;
    hdr.version = 3;
    hdr.length_samples = 3;

    core_create(&params, &ctx);

    std::vector<uint8_t> bytes(sizeof(hdr));
    std::memcpy(bytes.data(), &hdr, sizeof(hdr));
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    bytes.insert(bytes.end(), {0, 0, 0, 0});

    core_vcr_movie_header out_hdr{};
    vcr_read_movie_header(bytes, &out_hdr);

    REQUIRE(out_hdr.length_samples == 2);
}

/*
 * Tests that overriding inputs when idle using the `input` callback causes the correct overriden sample to be inputted.
 */
TEST_CASE("input_callback_override_works_when_idle", "vcr_on_controller_poll")
{
    prepare_test();

    params.callbacks.input = [](core_buttons *input, int index) { *input = {0xDEAD}; };

    core_create(&params, &ctx);
    vcr.task = task_idle;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == 0xDEAD);
}


TEST_CASE("input_callback_called_on_last_frame_of_movie", "vcr_on_controller_poll")
{
    prepare_test();

    static bool called = false;
    params.callbacks.input = [](core_buttons *input, int index) { called = true; };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 4;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

/*
 * Tests that overriding inputs during recording using the `input` callback causes the correct overriden sample to be
 * inputted.
 */
TEST_CASE("input_callback_override_works_when_recording", "vcr_on_controller_poll")
{
    prepare_test();

    params.callbacks.input = [](core_buttons *input, int index) { *input = {0xDEAD}; };

    core_create(&params, &ctx);
    vcr.inputs = {};
    vcr.hdr.length_samples = 0;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 0;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == 0xDEAD);
}

/*
 * Tests that overriding inputs during playback using the `input` callback causes the correct overriden sample to be
 * inputted.
 */
TEST_CASE("input_callback_override_works_when_playback", "vcr_on_controller_poll")
{
    prepare_test();

    params.callbacks.input = [](core_buttons *input, int index) { *input = {0xDEAD}; };

    core_create(&params, &ctx);
    vcr.inputs = {{1}, {2}, {3}, {4}};
    vcr.hdr.length_samples = vcr.inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 1;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == 0xDEAD);
}

/*
 * Tests that overriding inputs during recording using the `input` callback causes the correct overriden sample to be
 * appended to the inputs.
 */
TEST_CASE("correct_sample_appended_by_input_callback_override_during_recording", "vcr_on_controller_poll")
{
    prepare_test();

    params.callbacks.input = [](core_buttons *input, int index) { *input = {0xDEAD}; };

    core_create(&params, &ctx);
    vcr.inputs = {{1}, {2}, {3}, {4}};
    vcr.hdr.length_samples = vcr.inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = vcr.hdr.length_samples;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.inputs.back().value == 0xDEAD);
}

TEST_CASE("seek_stops_at_expected_frame", "seek")
{
    struct seek_test_params
    {
        t_vcr_state vcr{};
        std::string str{};
        size_t expected_frame{};
    };

    const auto param = GENERATE(
        seek_test_params{
            .vcr =
                {
                    .task = task_playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            core_buttons{0x01},
                            core_buttons{0x02},
                            core_buttons{0x03},
                            core_buttons{0x04},
                            core_buttons{0x05},
                        },
                    .current_sample = 0,
                },
            .str = "3",
            .expected_frame = 3,
        },

        seek_test_params{
            .vcr =
                {
                    .task = task_playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            core_buttons{0x01},
                            core_buttons{0x02},
                            core_buttons{0x03},
                            core_buttons{0x04},
                            core_buttons{0x05},
                        },
                    .current_sample = 3,
                },
            .str = "+1",
            .expected_frame = 4,
        },

        seek_test_params{
            .vcr =
                {
                    .task = task_playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            core_buttons{0x01},
                            core_buttons{0x02},
                            core_buttons{0x03},
                            core_buttons{0x04},
                            core_buttons{0x05},
                        },
                    .current_sample = 3,
                },
            .str = "-1",
            .expected_frame = 2,
        },

        seek_test_params{
            .vcr =
                {
                    .task = task_playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            core_buttons{0x01},
                            core_buttons{0x02},
                            core_buttons{0x03},
                            core_buttons{0x04},
                            core_buttons{0x05},
                        },
                    .current_sample = 3,
                },
            .str = "^1",
            .expected_frame = 4,
        });

    prepare_test();
    vcr = param.vcr;

    bool seek_completed = false;
    params.callbacks.seek_completed = [&] { seek_completed = true; };
    core_create(&params, &ctx);

    ctx->vr_start_rom = [](std::filesystem::path path) {
        emu_launched = true;
        core_executing = true;
        return Res_Ok;
    };

    ctx->vcr_start_playback = [](std::filesystem::path path) {
        vcr.task = task_playback;
        vcr.current_sample = 0;
        return Res_Ok;
    };

    const auto result = ctx->vcr_begin_seek(param.str, false);
    REQUIRE(result == Res_Ok);

    while (!seek_completed)
    {
        core_buttons input{};
        vcr_on_controller_poll(0, &input);
    }

    REQUIRE(vcr.current_sample == param.expected_frame + 1);
}

/*
 * Tests that the vcr_freeze function returns false when idle.
 */
TEST_CASE("returns_false_when_idle", "vcr_freeze")
{
    prepare_test();
    core_create(&params, &ctx);

    vcr_freeze_info freeze{};
    const auto result = vcr_freeze(freeze);

    REQUIRE(!result);
}

/*
 * Tests that the vcr_freeze function produces the correct freeze buffer for a predefined set of VCR states.
 */
TEST_CASE("out_freeze_is_correct", "vcr_freeze")
{
    struct freeze_test_params
    {
        t_vcr_state vcr{};
        vcr_freeze_info expected_freeze{};
    };

    const auto param = GENERATE(freeze_test_params{
        .vcr =
            {
                .task = task_playback,
                .hdr =
                    {
                        .uid = 0xDEAD,
                        .length_samples = 5,
                        .controller_flags = CONTROLLER_X_PRESENT(0),
                    },
                .inputs = {{1}, {2}, {3}, {4}, {5}},
                .current_sample = 2,
                .current_vi = 4,
            },
        .expected_freeze = {.size = 16 + 4 * 6,
                            .uid = 0xDEAD,
                            .current_sample = 2,
                            .current_vi = 4,
                            .length_samples = 5,
                            .input_buffer = {{1}, {2}, {3}, {4}, {5}, {0}}},
    });

    prepare_test();
    vcr = param.vcr;
    core_create(&params, &ctx);

    vcr_freeze_info freeze{};
    const auto result = vcr_freeze(freeze);

    REQUIRE(result);
    REQUIRE(freeze.size == param.expected_freeze.size);
    REQUIRE(freeze.uid == param.expected_freeze.uid);
    REQUIRE(freeze.current_sample == param.expected_freeze.current_sample);
    REQUIRE(freeze.current_vi == param.expected_freeze.current_vi);
    REQUIRE(freeze.length_samples == param.expected_freeze.length_samples);
    REQUIRE(freeze.input_buffer == param.expected_freeze.input_buffer);
}

/*
 * Tests that vcr_unfreeze fails with VCR_NeedsPlaybackOrRecording when called while idle.
 */
TEST_CASE("fails_when_idle", "vcr_unfreeze")
{
    prepare_test();
    core_create(&params, &ctx);

    vcr_freeze_info freeze{};
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == VCR_NeedsPlaybackOrRecording);
}

/*
 * Tests that vcr_unfreeze fails with VCR_InvalidFormat when the freeze buffer's size field is categorically too small.
 */
TEST_CASE("fails_when_size_too_small", "vcr_unfreeze")
{
    prepare_test();
    core_create(&params, &ctx);

    vcr.task = task_recording;

    vcr_freeze_info freeze{
        .size = 15,
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == VCR_InvalidFormat);
}

/*
 * Tests that vcr_unfreeze fails with VCR_NotFromThisMovie when the freeze buffer's uid field doesn't match the current
 * movie's uid.
 */
TEST_CASE("fails_when_uid_incompatible", "vcr_unfreeze")
{
    prepare_test();
    core_create(&params, &ctx);

    vcr.task = task_recording;
    vcr.hdr.uid = 0xBEEF;

    vcr_freeze_info freeze{
        .size = 16,
        .uid = 0xDEAD,
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == VCR_NotFromThisMovie);
}

/*
 * Tests that vcr_unfreeze fails with VCR_InvalidFrame when the freeze buffer is from a future sample of the current
 * movie, but the VCR is in read-only mode (which would cause a desync due to the input buffer not being updated and
 * therefore mismatched).
 */
TEST_CASE("fails_when_desync_risk", "vcr_unfreeze")
{
    prepare_test();
    core_create(&params, &ctx);

    cfg.vcr_readonly = true;

    vcr.task = task_recording;
    vcr.hdr.uid = 0xDEAD;

    vcr_freeze_info freeze{
        .size = 16,
        .uid = 0xDEAD,
        .current_sample = 10,
        .length_samples = 5,
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == VCR_InvalidFrame);
}

/*
 * Tests that vcr_unfreeze fails with VCR_InvalidFormat when the freeze buffer's size field is smaller than the expected
 * size for the given input buffer.
 */
TEST_CASE("fails_when_malformed_input_size", "vcr_unfreeze")
{
    prepare_test();
    core_create(&params, &ctx);

    cfg.vcr_readonly = false;

    vcr.task = task_recording;
    vcr.hdr.uid = 0xDEAD;

    vcr_freeze_info freeze{
        .size = 16 + sizeof(core_buttons) * 1,
        .uid = 0xDEAD,
        .current_sample = 10,
        .length_samples = 5,
        .input_buffer = {{1}, {2}},
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == VCR_InvalidFormat);
}

/*
 * Tests that vcr_unfreeze succeeds and updates the current sample but not the input buffer when unfreezing while
 * seeking and recording.
 */
TEST_CASE("input_buffer_doesnt_change_if_seeking_while_recording", "vcr_unfreeze")
{
    prepare_test();
    core_create(&params, &ctx);

    cfg.vcr_readonly = false;

    vcr.task = task_recording;
    vcr.hdr.uid = 0xDEAD;
    vcr.seek_to_frame = std::make_optional(1);
    vcr.current_sample = 2;
    vcr.inputs = {{0xDEAD}, {0xBEEF}, {0xCAFE}};

    vcr_freeze_info freeze{
        .size = 16 + sizeof(core_buttons) * 2,
        .uid = 0xDEAD,
        .current_sample = 0,
        .length_samples = 5,
        .input_buffer = {{1}, {2}},
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == Res_Ok);
    REQUIRE(vcr.inputs.size() == 3);
    REQUIRE(vcr.inputs[0].value == 0xDEAD);
    REQUIRE(vcr.inputs[1].value == 0xBEEF);
    REQUIRE(vcr.inputs[2].value == 0xCAFE);
    REQUIRE(vcr.current_sample == 0);
}

// TODO: More coverage for vcr_unfreeze!

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when idle.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_input_callback_called_while_idle", "vcr_on_controller_poll")
{
    prepare_test();
    params.callbacks.input = [&](core_buttons *input, int index) { REQUIRE(!is_vcr_lock_held()); };
    core_create(&params, &ctx);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when recording (standard appending
 * path). This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads
 * that also try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_input_callback_called_while_recording_1", "vcr_on_controller_poll")
{
    prepare_test();
    params.callbacks.input = [&](core_buttons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 4;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when recording (pseudo-playback
 * path). This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads
 * that also try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_input_callback_called_while_recording_2", "vcr_on_controller_poll")
{
    prepare_test();
    params.callbacks.input = [&](core_buttons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 2;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when playing back.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_input_callback_called_while_playback", "vcr_on_controller_poll")
{
    prepare_test();
    params.callbacks.input = [&](core_buttons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 3;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the vr_pause_emu call.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_emu_pause_changed_callback_called_while_playback_and_waiting_at_end",
          "vcr_on_controller_poll")
{
    prepare_test();

    bool called{};
    params.callbacks.emu_paused_changed = [&](const bool &) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    cfg.wait_at_movie_end = true;
    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 3;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

/*
 * Tests that stopping the VCR during an input callback while recording does not perform any recording work.
 */
TEST_CASE("stopping_vcr_during_input_callback_while_recording_doesnt_do_recording_work", "vcr_on_controller_poll")
{
    prepare_test();
    params.callbacks.input = [&](core_buttons *input, int index) { vcr_stop_all(); };

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}, {4}};

    core_create(&params, &ctx);

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 4;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.task == task_idle);
    REQUIRE(vcr.hdr.length_samples == inputs.size());
    REQUIRE(vcr.current_sample == 4);
}

/*
 * Tests that the VCR mutex is unlocked during callbacks invoked when calling vcr_stop_all with playback task.
 */
TEST_CASE("mutex_unlocked_during_callbacks_with_playback_task", "vcr_stop_all")
{
    prepare_test();

    bool task_changed_called = false;
    params.callbacks.task_changed = [&](auto) {
        task_changed_called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    bool stop_movie_called = false;
    params.callbacks.stop_movie = [&] {
        stop_movie_called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    core_create(&params, &ctx);

    vcr.task = task_playback;

    vcr_stop_all();

    REQUIRE(task_changed_called);
    REQUIRE(stop_movie_called);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the vr_pause_emu call.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_emu_paused_changed_callback_when_seek_ends", "vcr_on_controller_poll")
{
    prepare_test();

    bool called{};
    params.callbacks.emu_paused_changed = [&](const bool &) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    core_create(&params, &ctx);

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}};
    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_recording;
    vcr.current_sample = 1;
    vcr.seek_start_sample = 0;
    vcr.seek_to_frame = std::make_optional(1);
    vcr.seek_pause_at_end = true;

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

/*
 * Tests that vcr_on_vi unlocks the VCR mutex during the vr_pause_emu call.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE("mutex_unlocked_during_emu_paused_changed_callback", "vcr_on_vi")
{
    prepare_test();

    bool called{};
    params.callbacks.emu_paused_changed = [&](const bool &) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    core_create(&params, &ctx);

    cfg.pause_at_last_frame = true;

    const auto inputs = std::vector<core_buttons>{{1}, {2}, {3}};
    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = task_playback;
    vcr.current_sample = 3;

    vcr_on_vi();

    REQUIRE(called);
}

TEST_CASE("fails_when_not_playback", "vcr_continue_recording")
{
    prepare_test();
    core_create(&params, &ctx);

    vcr.task = task_idle;
    const auto result = vcr_continue_recording();
    REQUIRE(result == VCR_NeedsPlayback);
}

TEST_CASE("changes_task_and_header_and_inputs", "vcr_continue_recording")
{
    prepare_test();

    params.get_plugin_names = [](char *video, char *audio, char *input, char *rsp) {

    };
    
    core_create(&params, &ctx);

    cfg.vcr_backups = false;

    vcr.task = task_playback;
    vcr.hdr.length_samples = 5;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.inputs = {{1}, {2}, {3}, {4}, {5}};
    vcr.current_sample = 2;

    const auto result = vcr_continue_recording();

    REQUIRE(result == Res_Ok);
    REQUIRE(vcr.task == task_recording);
    REQUIRE(vcr.hdr.length_samples == 2);
    REQUIRE(vcr.inputs.size() == 2);
    REQUIRE(vcr.inputs[0].value == 1);
    REQUIRE(vcr.inputs[1].value == 2);
}


TEST_CASE("invokes_task_callback_correctly", "vcr_continue_recording")
{
    prepare_test();

    params.get_plugin_names = [](char *video, char *audio, char *input, char *rsp) {

    };
    bool called{};
    params.callbacks.task_changed = [&](const auto&) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    core_create(&params, &ctx);

    cfg.vcr_backups = false;

    vcr.task = task_playback;
    vcr.hdr.length_samples = 5;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.inputs = {{1}, {2}, {3}, {4}, {5}};
    vcr.current_sample = 2;

    const auto result = vcr_continue_recording();

    REQUIRE(called);
}

#pragma endregion
