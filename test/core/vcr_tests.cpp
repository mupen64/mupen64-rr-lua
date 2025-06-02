/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdafx.h>
#include <Core/r4300/vcr.h>
#include <Core/r4300/r4300.h>

extern t_vcr_state vcr;
static core_cfg cfg{};
static core_params params{};
static core_ctx* ctx = nullptr;
// static fakeit::Mock<IIOHelperService> io_service{};
static IIOHelperService real_io_helper_service{};

#pragma region Integration

static void prepare_test()
{
    // io_service.Reset();

    vcr = {};
    cfg = {};
    params.cfg = &cfg;
    // params.io_service = &io_service.get();
    params.io_service = &real_io_helper_service;
    params.plugin_funcs.input_get_keys = [](int32_t controller, core_buttons* keys) {
    };
    params.plugin_funcs.input_set_keys = [](int32_t controller, core_buttons keys) {
    };

    // fakeit::When(Method(io_service, str_nth_occurence)).AlwaysDo([&](const std::string& s, const std::string& delim, int n) {
    //     return real_io_helper_service.str_nth_occurence(s, delim, n);
    // });
    // fakeit::When(Method(io_service, string_to_wstring)).AlwaysDo([&](const auto& o) {
    //     return real_io_helper_service.string_to_wstring(o);
    // });
    // fakeit::When(Method(io_service, wstring_to_string)).AlwaysDo([&](const auto& o) {
    //     return real_io_helper_service.wstring_to_string(o);
    // });
}


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

    params.plugin_funcs.input_get_keys = [](int32_t index, core_buttons* input) {
        *input = {INPUT_VALUE};
    };
    core_create(&params, &ctx);

    core_buttons input{};
    vcr_on_controller_poll(0, &input);

    REQUIRE(input.value == INPUT_VALUE);
}

TEST_CASE("playback_returns_correct_input", "vcr_on_controller_poll")
{
    prepare_test();

    core_create(&params, &ctx);

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

    REQUIRE(input.value == inputs[2].value);
}

TEST_CASE("record_appends_input", "vcr_on_controller_poll")
{
    prepare_test();

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

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

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

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

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

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
    params.callbacks.input = [](core_buttons* input, int index) {
        called = true;
    };

    const auto inputs = std::vector<core_buttons>{
    {1},
    {2},
    {3},
    {4}};

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

    std::vector<uint8_t> bytes(sizeof(hdr));
    std::memcpy(bytes.data(), &hdr, sizeof(hdr));
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    bytes.insert(bytes.end(), {0, 0, 0, 0});

    core_vcr_movie_header out_hdr{};
    vcr_read_movie_header(bytes, &out_hdr);

    REQUIRE(out_hdr.length_samples == 2);
}

TEST_CASE("seek_stops_at_expected_frame", "seek")
{
    struct seek_test_params {
        t_vcr_state vcr{};
        std::wstring str{};
        size_t expected_frame{};
    };

    const auto param = GENERATE(seek_test_params{
                                .vcr = {
                                .task = task_playback,
                                .hdr = {
                                .magic = 0x1a34364d,
                                .version = 3,
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
                                .magic = 0x1a34364d,
                                .version = 3,
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
                                .str = L"+1",
                                .expected_frame = 4,
                                },

                                seek_test_params{
                                .vcr = {
                                .task = task_playback,
                                .hdr = {
                                .magic = 0x1a34364d,
                                .version = 3,
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
                                },

                                seek_test_params{
                                .vcr = {
                                .task = task_playback,
                                .hdr = {
                                .magic = 0x1a34364d,
                                .version = 3,
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
                                .str = L"^1",
                                .expected_frame = 4,
                                });

    prepare_test();
    vcr = param.vcr;

    bool seek_completed = false;
    params.callbacks.seek_completed = [&] {
        seek_completed = true;
    };
    core_create(&params, &ctx);


    ctx->vr_start_rom = [](const std::wstring& path) {
        emu_launched = true;
        core_executing = true;
        return Res_Ok;
    };

    ctx->vcr_start_playback = [](const std::wstring& path) {
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

#pragma endregion
