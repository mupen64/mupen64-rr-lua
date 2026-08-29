/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Core/R4300/VCR.hpp>
#include <Core/R4300/R4300.hpp>

static CoreCfg s_cfg{};
static CoreParams s_core_params{};
static CoreCtx *s_core_ctx = nullptr;

struct VcrFixture
{
    VcrFixture()
    {
        std::filesystem::remove("test.m64");
        std::filesystem::remove("test.st");
        std::filesystem::remove("test.cht");

        vcr = {};
        s_cfg = {};
        s_core_params.cfg = &s_cfg;
        s_core_params.input_get_keys = [](int32_t, CoreButtons *) {};
        s_core_params.input_set_keys = [](int32_t, CoreButtons) {};
        s_core_params.callbacks = {};
        core_create(&s_core_params, &s_core_ctx);
    }
};

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

TEST_CASE_METHOD(VcrFixture, "reset_pending_returns_unmodified_input", "vcr_on_controller_poll")
{
    const auto INPUT_VALUE = 0xDEAD;

    vcr.reset_pending = true;

    CoreButtons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE_METHOD(VcrFixture, "seek_savestate_loading_returns_unmodified_input", "vcr_on_controller_poll")
{
    const auto INPUT_VALUE = 0xDEAD;

    vcr.seek_savestate_loading = true;

    CoreButtons input = {INPUT_VALUE};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE_METHOD(VcrFixture, "idle_task_returns_input_from_getkeys", "vcr_on_controller_poll")
{
    const auto INPUT_VALUE = 0xDEAD;

    s_core_params.input_get_keys = [](int32_t index, CoreButtons *input) { *input = {INPUT_VALUE}; };

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE_METHOD(VcrFixture, "playback_returns_correct_input", "vcr_on_controller_poll")
{
    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 2;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == inputs[2].value);
}

/*
 * Tests that vcr_on_controller_poll returns the correct input for multiple controllers during playback.
 */
TEST_CASE_METHOD(VcrFixture, "playback_returns_correct_input_2", "vcr_on_controller_poll")
{
    const auto inputs = std::vector<CoreButtons>{
        {0},
        {0},
        {1},
        {1},
        {2},
        {2},
    };

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0) | CONTROLLER_X_PRESENT(1);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 0;

    CoreButtons input{};

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
TEST_CASE_METHOD(VcrFixture, "playback_returns_correct_input_3", "vcr_on_controller_poll")
{
    const auto inputs = std::vector<CoreButtons>{
        {0},
        {0},
        {0},
        {1},
        {1},
        {1},
        {2},
        {2},
        {2},
    };

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0) | CONTROLLER_X_PRESENT(2) | CONTROLLER_X_PRESENT(3);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 0;

    CoreButtons input{};

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

TEST_CASE_METHOD(VcrFixture, "record_appends_input", "vcr_on_controller_poll")
{
    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 4;

    CoreButtons input{0xDEAD};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.inputs.back().value == 0xDEAD);
}

TEST_CASE_METHOD(VcrFixture, "seek_continues_when_end_not_reached", "vcr_on_controller_poll")
{
    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 1;
    vcr.seek_to_frame = std::make_optional(3);

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.seek_to_frame.has_value());
}

TEST_CASE_METHOD(VcrFixture, "seek_stops_when_end_reached", "vcr_on_controller_poll")
{
    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 3;
    vcr.seek_to_frame = std::make_optional(3);

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(!vcr.seek_to_frame.has_value());
}

TEST_CASE_METHOD(VcrFixture, "produces_correct_paths_with_start_type_from_start", "vcr_get_generated_file_info")
{
    CoreVCRGeneratedFileInfo info = vcr_get_generated_file_info("test.m64", MOVIE_START_FROM_NOTHING);

    REQUIRE(info.movie_path == "test.m64");
    REQUIRE(info.st_path.empty());
    REQUIRE(info.cht_path.empty());
}

TEST_CASE_METHOD(VcrFixture, "produces_correct_paths_with_start_type_from_savestate", "vcr_get_generated_file_info")
{
    CoreVCRGeneratedFileInfo info = vcr_get_generated_file_info("test.m64", MOVIE_START_FROM_SNAPSHOT);

    REQUIRE(info.movie_path == "test.m64");
    REQUIRE(info.st_path == "test.st");
    REQUIRE(info.cht_path.empty());
}

TEST_CASE_METHOD(VcrFixture, "produces_correct_paths_with_start_type_from_eeprom", "vcr_get_generated_file_info")
{
    CoreVCRGeneratedFileInfo info = vcr_get_generated_file_info("test.m64", MOVIE_START_FROM_EEPROM);

    REQUIRE(info.movie_path == "test.m64");
    REQUIRE(info.st_path.empty());
    REQUIRE(info.cht_path.empty());
}

TEST_CASE_METHOD(VcrFixture, "produces_correct_paths_with_cheats", "vcr_get_generated_file_info")
{
    CoreCheat cheat{};
    s_core_ctx->cht_set_list({cheat});

    CoreVCRGeneratedFileInfo info = vcr_get_generated_file_info("test.m64", MOVIE_START_FROM_EEPROM);

    REQUIRE(info.movie_path == "test.m64");
    REQUIRE(info.st_path.empty());
    REQUIRE(info.cht_path == "test.cht");
}

#pragma endregion

#pragma region Unit

// ee2a0e4
TEST_CASE_METHOD(VcrFixture, "input_callback_called_when_using_input_buffer_during_recording", "vcr_on_controller_poll")
{
    static bool called = false;
    s_core_params.callbacks.input = [](CoreButtons *input, int index) { called = true; };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 2;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

// 94e3d9d
TEST_CASE_METHOD(VcrFixture, "sample_length_gets_clamped_to_buffer_max", "read_movie_header")
{
    CoreVCRMovieHeader hdr{};
    hdr.magic = 0x1a34364d;
    hdr.version = 3;
    hdr.length_samples = 3;

    std::vector<uint8_t> bytes(sizeof(hdr));
    std::memcpy(bytes.data(), &hdr, sizeof(hdr));
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    bytes.insert(bytes.end(), {0, 0, 0, 0});

    CoreVCRMovieHeader out_hdr{};
    vcr_read_movie_header(bytes, &out_hdr);

    REQUIRE(out_hdr.length_samples == 2);
}

/*
 * Tests that overriding inputs when idle using the `input` callback causes the correct overriden sample to be inputted.
 */
TEST_CASE_METHOD(VcrFixture, "input_callback_override_works_when_idle", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [](CoreButtons *input, int index) { *input = {0xDEAD}; };
    vcr.task = CoreVCRTask::Idle;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == 0xDEAD);
}

TEST_CASE_METHOD(VcrFixture, "input_callback_called_on_last_frame_of_movie", "vcr_on_controller_poll")
{
    static bool called = false;
    s_core_params.callbacks.input = [](CoreButtons *input, int index) { called = true; };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 4;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

/*
 * Tests that overriding inputs during recording using the `input` callback causes the correct overriden sample to be
 * inputted.
 */
TEST_CASE_METHOD(VcrFixture, "input_callback_override_works_when_recording", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [](CoreButtons *input, int index) { *input = {0xDEAD}; };
    vcr.inputs = {};
    vcr.hdr.length_samples = 0;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 0;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == 0xDEAD);
}

/*
 * Tests that overriding inputs during playback using the `input` callback causes the correct overriden sample to be
 * inputted.
 */
TEST_CASE_METHOD(VcrFixture, "input_callback_override_works_when_playback", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [](CoreButtons *input, int index) { *input = {0xDEAD}; };
    vcr.inputs = {{1}, {2}, {3}, {4}};
    vcr.hdr.length_samples = vcr.inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 1;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == 0xDEAD);
}

/*
 * Tests that overriding inputs during recording using the `input` callback causes the correct overriden sample to be
 * appended to the inputs.
 */
TEST_CASE_METHOD(
    VcrFixture, "correct_sample_appended_by_input_callback_override_during_recording", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [](CoreButtons *input, int index) { *input = {0xDEAD}; };
    vcr.inputs = {{1}, {2}, {3}, {4}};
    vcr.hdr.length_samples = vcr.inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = vcr.hdr.length_samples;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.inputs.back().value == 0xDEAD);
}

TEST_CASE_METHOD(VcrFixture, "seek_stops_at_expected_frame", "seek")
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
                    .task = CoreVCRTask::Playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            CoreButtons{0x01},
                            CoreButtons{0x02},
                            CoreButtons{0x03},
                            CoreButtons{0x04},
                            CoreButtons{0x05},
                        },
                    .current_sample = 0,
                },
            .str = "3",
            .expected_frame = 3,
        },

        seek_test_params{
            .vcr =
                {
                    .task = CoreVCRTask::Playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            CoreButtons{0x01},
                            CoreButtons{0x02},
                            CoreButtons{0x03},
                            CoreButtons{0x04},
                            CoreButtons{0x05},
                        },
                    .current_sample = 3,
                },
            .str = "+1",
            .expected_frame = 4,
        },

        seek_test_params{
            .vcr =
                {
                    .task = CoreVCRTask::Playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            CoreButtons{0x01},
                            CoreButtons{0x02},
                            CoreButtons{0x03},
                            CoreButtons{0x04},
                            CoreButtons{0x05},
                        },
                    .current_sample = 3,
                },
            .str = "-1",
            .expected_frame = 2,
        },

        seek_test_params{
            .vcr =
                {
                    .task = CoreVCRTask::Playback,
                    .hdr =
                        {
                            .magic = 0x1a34364d,
                            .version = 3,
                            .length_samples = 5,
                            .controller_flags = CONTROLLER_X_PRESENT(0),
                        },
                    .inputs =
                        {
                            CoreButtons{0x01},
                            CoreButtons{0x02},
                            CoreButtons{0x03},
                            CoreButtons{0x04},
                            CoreButtons{0x05},
                        },
                    .current_sample = 3,
                },
            .str = "^1",
            .expected_frame = 4,
        });

    vcr = param.vcr;

    bool seek_completed = false;
    s_core_params.callbacks.seek_completed = [&] { seek_completed = true; };

    s_core_ctx->vr_start_rom = [](std::filesystem::path path) {
        emu_launched = true;
        core_executing = true;
        return CoreResult::Res_Ok;
    };

    s_core_ctx->vcr_start_playback = [](std::filesystem::path path) {
        vcr.task = CoreVCRTask::Playback;
        vcr.current_sample = 0;
        return CoreResult::Res_Ok;
    };

    const auto result = s_core_ctx->vcr_begin_seek(param.str, false);
    REQUIRE(result == CoreResult::Res_Ok);

    while (!seek_completed)
    {
        CoreButtons input{};
        vcr_on_controller_poll(0, &input);
    }

    REQUIRE(vcr.current_sample == param.expected_frame + 1);
}

/*
 * Tests that the vcr_freeze function returns false when idle.
 */
TEST_CASE_METHOD(VcrFixture, "returns_false_when_idle", "vcr_freeze")
{
    vcr_freeze_info freeze{};
    const auto result = vcr_freeze(freeze);

    REQUIRE(!result);
}

/*
 * Tests that the vcr_freeze function produces the correct freeze buffer for a predefined set of VCR states.
 */
TEST_CASE_METHOD(VcrFixture, "out_freeze_is_correct", "vcr_freeze")
{
    struct freeze_test_params
    {
        t_vcr_state vcr{};
        vcr_freeze_info expected_freeze{};
    };

    const auto param = GENERATE(freeze_test_params{
        .vcr =
            {
                .task = CoreVCRTask::Playback,
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

    vcr = param.vcr;

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
 * Tests that vcr_unfreeze fails with CoreResult::VCR_NeedsPlaybackOrRecording when called while idle.
 */
TEST_CASE_METHOD(VcrFixture, "fails_when_idle", "vcr_unfreeze")
{
    vcr_freeze_info freeze{};
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == CoreResult::VCR_NeedsPlaybackOrRecording);
}

/*
 * Tests that vcr_unfreeze fails with CoreResult::VCR_InvalidFormat when the freeze buffer's size field is categorically too small.
 */
TEST_CASE_METHOD(VcrFixture, "fails_when_size_too_small", "vcr_unfreeze")
{
    vcr.task = CoreVCRTask::Recording;

    vcr_freeze_info freeze{
        .size = 15,
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == CoreResult::VCR_InvalidFormat);
}

/*
 * Tests that vcr_unfreeze fails with CoreResult::VCR_NotFromThisMovie when the freeze buffer's uid field doesn't match the current
 * movie's uid.
 */
TEST_CASE_METHOD(VcrFixture, "fails_when_uid_incompatible", "vcr_unfreeze")
{
    vcr.task = CoreVCRTask::Recording;
    vcr.hdr.uid = 0xBEEF;

    vcr_freeze_info freeze{
        .size = 16,
        .uid = 0xDEAD,
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == CoreResult::VCR_NotFromThisMovie);
}

/*
 * Tests that vcr_unfreeze fails with CoreResult::VCR_InvalidFrame when the freeze buffer is from a future sample of the current
 * movie, but the VCR is in read-only mode (which would cause a desync due to the input buffer not being updated and
 * therefore mismatched).
 */
TEST_CASE_METHOD(VcrFixture, "fails_when_desync_risk", "vcr_unfreeze")
{
    s_cfg.vcr_readonly = true;

    vcr.task = CoreVCRTask::Recording;
    vcr.hdr.uid = 0xDEAD;

    vcr_freeze_info freeze{
        .size = 16,
        .uid = 0xDEAD,
        .current_sample = 10,
        .length_samples = 5,
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == CoreResult::VCR_InvalidFrame);
}

/*
 * Tests that vcr_unfreeze fails with CoreResult::VCR_InvalidFormat when the freeze buffer's size field is smaller than the expected
 * size for the given input buffer.
 */
TEST_CASE_METHOD(VcrFixture, "fails_when_malformed_input_size", "vcr_unfreeze")
{
    s_cfg.vcr_readonly = false;

    vcr.task = CoreVCRTask::Recording;
    vcr.hdr.uid = 0xDEAD;

    vcr_freeze_info freeze{
        .size = 16 + sizeof(CoreButtons) * 1,
        .uid = 0xDEAD,
        .current_sample = 10,
        .length_samples = 5,
        .input_buffer = {{1}, {2}},
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == CoreResult::VCR_InvalidFormat);
}

/*
 * Tests that vcr_unfreeze succeeds and updates the current sample but not the input buffer when unfreezing while
 * seeking and recording.
 */
TEST_CASE_METHOD(VcrFixture, "input_buffer_doesnt_change_if_seeking_while_recording", "vcr_unfreeze")
{
    s_cfg.vcr_readonly = false;

    vcr.task = CoreVCRTask::Recording;
    vcr.hdr.uid = 0xDEAD;
    vcr.seek_to_frame = std::make_optional(1);
    vcr.current_sample = 2;
    vcr.inputs = {{0xDEAD}, {0xBEEF}, {0xCAFE}};

    vcr_freeze_info freeze{
        .size = 16 + sizeof(CoreButtons) * 2,
        .uid = 0xDEAD,
        .current_sample = 0,
        .length_samples = 5,
        .input_buffer = {{1}, {2}},
    };
    const auto result = vcr_unfreeze(freeze);

    REQUIRE(result == CoreResult::Res_Ok);
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
TEST_CASE_METHOD(VcrFixture, "mutex_unlocked_during_input_callback_called_while_idle", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [&](CoreButtons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when recording (standard appending
 * path). This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads
 * that also try to lock the VCR mutex.
 */
TEST_CASE_METHOD(VcrFixture, "mutex_unlocked_during_input_callback_called_while_recording_1", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [&](CoreButtons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 4;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when recording (pseudo-playback
 * path). This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads
 * that also try to lock the VCR mutex.
 */
TEST_CASE_METHOD(VcrFixture, "mutex_unlocked_during_input_callback_called_while_recording_2", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [&](CoreButtons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 2;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the input callback when playing back.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE_METHOD(VcrFixture, "mutex_unlocked_during_input_callback_called_while_playback", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [&](CoreButtons *input, int index) { REQUIRE(!is_vcr_lock_held()); };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 3;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the vr_pause_emu call.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE_METHOD(VcrFixture,
    "mutex_unlocked_during_emu_pause_changed_callback_called_while_playback_and_waiting_at_end",
    "vcr_on_controller_poll")
{
    bool called{};
    s_core_params.callbacks.emu_paused_changed = [&](const bool &) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    s_cfg.wait_at_movie_end = true;
    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Playback;
    vcr.current_sample = 3;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

/*
 * Tests that stopping the VCR during an input callback while recording does not perform any recording work.
 */
TEST_CASE_METHOD(
    VcrFixture, "stopping_vcr_during_input_callback_while_recording_doesnt_do_recording_work", "vcr_on_controller_poll")
{
    s_core_params.callbacks.input = [&](CoreButtons *input, int index) { vcr_stop_all(); };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}, {4}};

    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 4;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(vcr.task == CoreVCRTask::Idle);
    REQUIRE(vcr.hdr.length_samples == inputs.size());
    REQUIRE(vcr.current_sample == 4);
}

/*
 * Tests that the VCR mutex is unlocked during callbacks invoked when calling vcr_stop_all with playback task.
 */
TEST_CASE_METHOD(VcrFixture, "mutex_unlocked_during_callbacks_with_playback_task", "vcr_stop_all")
{
    bool task_changed_called = false;
    s_core_params.callbacks.task_changed = [&](auto) {
        task_changed_called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    bool stop_movie_called = false;
    s_core_params.callbacks.stop_movie = [&] {
        stop_movie_called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    vcr.task = CoreVCRTask::Playback;

    vcr_stop_all();

    REQUIRE(task_changed_called);
    REQUIRE(stop_movie_called);
}

/*
 * Tests that vcr_on_controller_poll unlocks the VCR mutex during the vr_pause_emu call.
 * This is important to avoid deadlocks when the input callback dispatches synchronous calls to other threads that also
 * try to lock the VCR mutex.
 */
TEST_CASE_METHOD(
    VcrFixture, "mutex_unlocked_during_emu_paused_changed_callback_when_seek_ends", "vcr_on_controller_poll")
{
    bool called{};
    s_core_params.callbacks.emu_paused_changed = [&](const bool &) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    const auto inputs = std::vector<CoreButtons>{{1}, {2}, {3}};
    vcr.inputs = inputs;
    vcr.hdr.length_samples = inputs.size();
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.task = CoreVCRTask::Recording;
    vcr.current_sample = 1;
    vcr.seek_start_sample = 0;
    vcr.seek_to_frame = std::make_optional(1);
    vcr.seek_pause_at_end = true;

    CoreButtons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(called);
}

TEST_CASE_METHOD(VcrFixture, "fails_when_not_playback", "vcr_continue_recording")
{
    vcr.task = CoreVCRTask::Idle;
    const auto result = vcr_continue_recording();
    REQUIRE(result == CoreResult::VCR_NeedsPlayback);
}

TEST_CASE_METHOD(VcrFixture, "changes_task_and_header_and_inputs", "vcr_continue_recording")
{
    s_core_params.get_plugin_names = [](char *video, char *audio, char *input, char *rsp) {};

    s_cfg.vcr_backups = false;

    vcr.task = CoreVCRTask::Playback;
    vcr.hdr.length_samples = 5;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.inputs = {{1}, {2}, {3}, {4}, {5}};
    vcr.current_sample = 2;

    const auto result = vcr_continue_recording();

    REQUIRE(result == CoreResult::Res_Ok);
    REQUIRE(vcr.task == CoreVCRTask::Recording);
    REQUIRE(vcr.hdr.length_samples == 2);
    REQUIRE(vcr.inputs.size() == 2);
    REQUIRE(vcr.inputs[0].value == 1);
    REQUIRE(vcr.inputs[1].value == 2);
}

TEST_CASE_METHOD(VcrFixture, "invokes_task_callback_correctly", "vcr_continue_recording")
{
    s_core_params.get_plugin_names = [](char *video, char *audio, char *input, char *rsp) {};
    bool called{};
    s_core_params.callbacks.task_changed = [&](const auto &) {
        called = true;
        REQUIRE(!is_vcr_lock_held());
    };

    s_cfg.vcr_backups = false;

    vcr.task = CoreVCRTask::Playback;
    vcr.hdr.length_samples = 5;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.inputs = {{1}, {2}, {3}, {4}, {5}};
    vcr.current_sample = 2;

    const auto result = vcr_continue_recording();

    REQUIRE(called);
}

TEST_CASE_METHOD(VcrFixture, "doesnt_deadlock", "vcr_begin_warp_modify")
{
    s_cfg.vcr_backups = false;

    vcr.task = CoreVCRTask::Recording;
    vcr.hdr.length_samples = 5;
    vcr.hdr.controller_flags = CONTROLLER_X_PRESENT(0);
    vcr.inputs = {{1}, {2}, {3}, {4}, {5}};
    vcr.current_sample = 4;

    const auto result = vcr_begin_warp_modify({{0}, {0}, {0}, {0}});

    REQUIRE(result == CoreResult::Res_Ok);
}

TEST_CASE_METHOD(VcrFixture, "returns_correct_sync_data_for_extended_version_0", "vcr_get_sync_data_from_header")
{
    CoreVCRMovieHeader header{};
    header.extended_version = 0;
    header.extended_flags.wii_vc = true;
    header.extended_flags.c_eq_s_accurate = true;
    header.extended_flags.accurate_rdp_completion = true;
    header.extended_data.cpu_cf = 2.0;
    header.extended_data.rcp_lag_factor = 1.5;

    const auto sync_data = vcr_get_sync_data_from_header(header);

    REQUIRE(sync_data.has_value());
    REQUIRE(sync_data->wii_vc == false);
    REQUIRE(sync_data->c_eq_s_accurate == false);
    REQUIRE(sync_data->accurate_rdp_completion == false);
    REQUIRE(sync_data->cpu_cf == 1.0);
    REQUIRE(sync_data->rcp_lag_factor == 0.0);
}

TEST_CASE_METHOD(VcrFixture, "returns_correct_sync_data_for_extended_version_1", "vcr_get_sync_data_from_header")
{
    CoreVCRMovieHeader header{};
    header.extended_version = 1;
    header.extended_flags.wii_vc = true;

    const auto sync_data = vcr_get_sync_data_from_header(header);

    REQUIRE(sync_data.has_value());
    REQUIRE(sync_data->wii_vc == true);
    REQUIRE(sync_data->c_eq_s_accurate == false);
    REQUIRE(sync_data->accurate_rdp_completion == false);
    REQUIRE(sync_data->cpu_cf == 1.0);
    REQUIRE(sync_data->rcp_lag_factor == 0.0);
}

TEST_CASE_METHOD(VcrFixture, "returns_correct_sync_data_for_extended_version_2", "vcr_get_sync_data_from_header")
{
    CoreVCRMovieHeader header{};
    header.extended_version = 2;
    header.extended_flags.wii_vc = true;
    header.extended_flags.c_eq_s_accurate = true;

    const auto sync_data = vcr_get_sync_data_from_header(header);

    REQUIRE(sync_data.has_value());
    REQUIRE(sync_data->wii_vc == true);
    REQUIRE(sync_data->c_eq_s_accurate == true);
    REQUIRE(sync_data->accurate_rdp_completion == false);
    REQUIRE(sync_data->cpu_cf == 1.0);
    REQUIRE(sync_data->rcp_lag_factor == 0.0);
}

TEST_CASE_METHOD(VcrFixture, "returns_correct_sync_data_for_extended_version_3", "vcr_get_sync_data_from_header")
{
    CoreVCRMovieHeader header{};
    header.extended_version = 3;
    header.extended_flags.wii_vc = false;
    header.extended_flags.c_eq_s_accurate = true;
    header.extended_flags.accurate_rdp_completion = true;

    const auto sync_data = vcr_get_sync_data_from_header(header);

    REQUIRE(sync_data.has_value());
    REQUIRE(sync_data->wii_vc == false);
    REQUIRE(sync_data->c_eq_s_accurate == true);
    REQUIRE(sync_data->accurate_rdp_completion == true);
    REQUIRE(sync_data->cpu_cf == 1.0);
    REQUIRE(sync_data->rcp_lag_factor == 0.0);
}

TEST_CASE_METHOD(VcrFixture, "returns_correct_sync_data_for_extended_version_4", "vcr_get_sync_data_from_header")
{
    CoreVCRMovieHeader header{};
    header.extended_version = 4;
    header.extended_flags.wii_vc = true;
    header.extended_flags.c_eq_s_accurate = false;
    header.extended_flags.accurate_rdp_completion = true;
    header.extended_data.cpu_cf = 2.0;
    header.extended_data.rcp_lag_factor = 1.5;

    const auto sync_data = vcr_get_sync_data_from_header(header);

    REQUIRE(sync_data.has_value());
    REQUIRE(sync_data->wii_vc == true);
    REQUIRE(sync_data->c_eq_s_accurate == false);
    REQUIRE(sync_data->accurate_rdp_completion == true);
    REQUIRE(sync_data->cpu_cf == 2.0);
    REQUIRE(sync_data->rcp_lag_factor == 1.5);
}

TEST_CASE_METHOD(VcrFixture, "returns_nullopt_for_invalid_extended_version", "vcr_get_sync_data_from_header")
{
    CoreVCRMovieHeader header{};
    header.extended_version = 5;

    const auto sync_data = vcr_get_sync_data_from_header(header);

    REQUIRE(!sync_data.has_value());
}

TEST_CASE_METHOD(VcrFixture, "returns_no_warnings_when_sync_data_matches_config", "vcr_get_sync_warnings")
{
    s_cfg.wii_vc_emulation = true;
    s_cfg.core_type = 1;
    s_cfg.c_eq_s_nan_accurate = true;
    s_cfg.accurate_rdp_completion = true;
    s_cfg.cpu_cf = 2.0;
    s_cfg.rcp_lag_factor = 1.5;
    s_cfg.rcp_lag_emulation = true;

    SyncData sync_data{
        .wii_vc = true,
        .c_eq_s_accurate = true,
        .accurate_rdp_completion = true,
        .cpu_cf = 2.0,
        .rcp_lag_factor = 1.5,
    };

    const auto warnings = vcr_get_sync_warnings(sync_data);

    REQUIRE(warnings.empty());
}

TEST_CASE_METHOD(VcrFixture, "returns_warnings_for_unspecified_optional_fields", "vcr_get_sync_warnings")
{
    s_cfg.wii_vc_emulation = false;
    s_cfg.accurate_rdp_completion = false;

    SyncData sync_data{
        .wii_vc = false,
        .c_eq_s_accurate = std::nullopt,
        .accurate_rdp_completion = false,
        .cpu_cf = std::nullopt,
        .rcp_lag_factor = std::nullopt,
    };

    const auto warnings = vcr_get_sync_warnings(sync_data);

    REQUIRE(warnings.size() == 3);
    REQUIRE(warnings[0] == "C.EQ.S correctness not specified in movie.");
    REQUIRE(warnings[1] == "CPU counter factor not specified in movie.");
    REQUIRE(warnings[2] == "RCP lag factor not specified in movie.");
}

TEST_CASE_METHOD(VcrFixture, "returns_warnings_when_bool_flags_mismatch_config", "vcr_get_sync_warnings")
{
    s_cfg.wii_vc_emulation = false;
    s_cfg.core_type = 1;
    s_cfg.c_eq_s_nan_accurate = true;
    s_cfg.accurate_rdp_completion = false;
    s_cfg.cpu_cf = 1.0;
    s_cfg.rcp_lag_factor = 1.0;
    s_cfg.rcp_lag_emulation = true;

    SyncData sync_data{
        .wii_vc = true,
        .c_eq_s_accurate = false,
        .accurate_rdp_completion = true,
        .cpu_cf = 1.0,
        .rcp_lag_factor = 1.0,
    };

    const auto warnings = vcr_get_sync_warnings(sync_data);

    REQUIRE(warnings.size() == 3);
    REQUIRE(warnings[0] == "WiiVC mode enabled in movie, but disabled in emulator.");
    REQUIRE(warnings[1] == "C.EQ.S correctness disabled in movie, but enabled in emulator.");
    REQUIRE(warnings[2] == "RDP completion accuracy enabled in movie, but disabled in emulator.");
}

TEST_CASE_METHOD(VcrFixture, "returns_warnings_when_double_values_mismatch_config", "vcr_get_sync_warnings")
{
    s_cfg.wii_vc_emulation = false;
    s_cfg.core_type = 1;
    s_cfg.c_eq_s_nan_accurate = false;
    s_cfg.accurate_rdp_completion = false;
    s_cfg.cpu_cf = 2.5;
    s_cfg.rcp_lag_factor = 1.5;
    s_cfg.rcp_lag_emulation = true;

    SyncData sync_data{
        .wii_vc = false,
        .c_eq_s_accurate = false,
        .accurate_rdp_completion = false,
        .cpu_cf = 1.5,
        .rcp_lag_factor = 0.5,
    };

    const auto warnings = vcr_get_sync_warnings(sync_data);

    REQUIRE(warnings.size() == 2);
    REQUIRE(warnings[0] == "CPU counter factor 1.5 in movie, but 2.5 in emulator.");
    REQUIRE(warnings[1] == "RCP lag factor 0.5 in movie, but 1.5 in emulator.");
}

TEST_CASE_METHOD(VcrFixture, "returns_no_warnings_when_rcp_lag_factor_mismatch_without_lag_emulation_enabled",
    "vcr_get_sync_warnings")
{
    s_cfg.wii_vc_emulation = false;
    s_cfg.core_type = 1;
    s_cfg.c_eq_s_nan_accurate = false;
    s_cfg.accurate_rdp_completion = false;
    s_cfg.cpu_cf = 1.0;
    s_cfg.rcp_lag_factor = 2.0;
    s_cfg.rcp_lag_emulation = 0;

    SyncData sync_data{
        .wii_vc = false,
        .c_eq_s_accurate = false,
        .accurate_rdp_completion = false,
        .cpu_cf = 1.0,
        .rcp_lag_factor = 0.0,
    };

    const auto warnings = vcr_get_sync_warnings(sync_data);

    REQUIRE(warnings.size() == 0);
}

namespace
{
constexpr auto REPLACE_MOVIE_PATH = "test_replace_author.m64";

void create_test_movie(const std::string &author, const std::string &description)
{
    CoreVCRMovieHeader hdr{};
    hdr.magic = 0x1a34364d;
    hdr.version = 3;
    hdr.length_samples = 0;

    std::vector<uint8_t> bytes(sizeof(hdr));
    std::memcpy(bytes.data(), &hdr, sizeof(hdr));
    std::memcpy(bytes.data() + 0x222, author.data(), author.size());
    std::memcpy(bytes.data() + 0x300, description.data(), description.size());

    REQUIRE(IOUtils::write_entire_file(REPLACE_MOVIE_PATH, bytes));
}

std::string read_movie_field(size_t offset, size_t size)
{
    const auto buf = IOUtils::read_entire_file(REPLACE_MOVIE_PATH);
    REQUIRE(buf.size() >= offset + size);

    std::string out(reinterpret_cast<const char *>(buf.data() + offset), size);
    out.resize(std::strlen(out.c_str()));
    return out;
}

std::string read_movie_author()
{
    return read_movie_field(0x222, 222);
}

std::string read_movie_description()
{
    return read_movie_field(0x300, 256);
}
} // namespace

TEST_CASE_METHOD(VcrFixture, "replaces_author_only_keeps_description", "vcr_replace_author_info")
{
    create_test_movie("old author", "old description");

    const auto result = vcr_replace_author_info(REPLACE_MOVIE_PATH, std::string("new author"), std::nullopt);

    REQUIRE(result == CoreResult::Res_Ok);
    REQUIRE(read_movie_author() == "new author");
    REQUIRE(read_movie_description() == "old description");
}

TEST_CASE_METHOD(VcrFixture, "replaces_description_only_keeps_author", "vcr_replace_author_info")
{
    create_test_movie("old author", "old description");

    const auto result = vcr_replace_author_info(REPLACE_MOVIE_PATH, std::nullopt, std::string("new description"));

    REQUIRE(result == CoreResult::Res_Ok);
    REQUIRE(read_movie_author() == "old author");
    REQUIRE(read_movie_description() == "new description");
}

TEST_CASE_METHOD(VcrFixture, "replaces_author_and_description", "vcr_replace_author_info")
{
    create_test_movie("old author", "old description");

    const auto result =
        vcr_replace_author_info(REPLACE_MOVIE_PATH, std::string("new author"), std::string("new description"));

    REQUIRE(result == CoreResult::Res_Ok);
    REQUIRE(read_movie_author() == "new author");
    REQUIRE(read_movie_description() == "new description");
}

TEST_CASE_METHOD(VcrFixture, "nullopt_fields_leave_movie_unchanged", "vcr_replace_author_info")
{
    create_test_movie("old author", "old description");

    const auto result = vcr_replace_author_info(REPLACE_MOVIE_PATH, std::nullopt, std::nullopt);

    REQUIRE(result == CoreResult::Res_Ok);
    REQUIRE(read_movie_author() == "old author");
    REQUIRE(read_movie_description() == "old description");
}

TEST_CASE_METHOD(VcrFixture, "identical_values_leave_movie_unchanged", "vcr_replace_author_info")
{
    create_test_movie("same author", "same description");

    const auto result =
        vcr_replace_author_info(REPLACE_MOVIE_PATH, std::string("same author"), std::string("same description"));

    REQUIRE(result == CoreResult::Res_Ok);
    REQUIRE(read_movie_author() == "same author");
    REQUIRE(read_movie_description() == "same description");
}

TEST_CASE_METHOD(VcrFixture, "author_longer_than_222_returns_invalid_format", "vcr_replace_author_info")
{
    create_test_movie("old author", "old description");

    const auto result = vcr_replace_author_info(REPLACE_MOVIE_PATH, std::string(223, 'a'), std::nullopt);

    REQUIRE(result == CoreResult::VCR_InvalidFormat);
    REQUIRE(read_movie_author() == "old author");
}

TEST_CASE_METHOD(VcrFixture, "description_longer_than_256_returns_invalid_format", "vcr_replace_author_info")
{
    create_test_movie("old author", "old description");

    const auto result = vcr_replace_author_info(REPLACE_MOVIE_PATH, std::nullopt, std::string(257, 'b'));

    REQUIRE(result == CoreResult::VCR_InvalidFormat);
    REQUIRE(read_movie_description() == "old description");
}

TEST_CASE_METHOD(VcrFixture, "missing_file_returns_bad_file", "vcr_replace_author_info")
{
    std::filesystem::remove(REPLACE_MOVIE_PATH);

    const auto result = vcr_replace_author_info(REPLACE_MOVIE_PATH, std::string("new author"), std::nullopt);

    REQUIRE(result == CoreResult::VCR_BadFile);
}

#pragma endregion
