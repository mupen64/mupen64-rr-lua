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

struct VrFixture
{
    VrFixture()
    {
        vcr = {};
        s_cfg = {};
        s_core_params = {};
        s_core_params.cfg = &s_cfg;
        s_core_params.input_get_keys = [](int32_t, CoreButtons *) {};
        s_core_params.input_set_keys = [](int32_t, CoreButtons) {};
        s_core_params.callbacks = {};
        core_create(&s_core_params, &s_core_ctx);

        g_r4300.desired_speed_mode.store(CoreSpeedMode::Normal);
        g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
        g_r4300.frame_advance_outstanding.store(0);
    }
};

TEST_CASE_METHOD(VrFixture, "frame_advance_outstanding_over_one_uses_ultra_fast_forward_when_render_throttling_enabled",
    "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 1;
    g_r4300.frame_advance_outstanding.store(2);

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::UltraFastForward);
}

TEST_CASE_METHOD(VrFixture, "frame_advance_outstanding_over_one_uses_fast_forward_when_render_throttling_disabled",
    "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 0;
    g_r4300.frame_advance_outstanding.store(2);

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::FastForward);
}

TEST_CASE_METHOD(
    VrFixture, "seek_to_frame_uses_ultra_fast_forward_when_render_throttling_enabled", "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 1;
    vcr.seek_to_frame = 7;

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::UltraFastForward);
}

TEST_CASE_METHOD(
    VrFixture, "seek_to_frame_uses_fast_forward_when_render_throttling_disabled", "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 0;
    vcr.seek_to_frame = 7;

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::FastForward);
}

TEST_CASE_METHOD(VrFixture, "desired_speed_mode_uses_requested_value_when_not_normal", "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 1;
    g_r4300.desired_speed_mode.store(CoreSpeedMode::FastForward);

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::FastForward);
}

TEST_CASE_METHOD(VrFixture, "desired_speed_mode_ultra_fast_forward_downgraded_when_render_throttling_disabled",
    "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 0;
    g_r4300.desired_speed_mode.store(CoreSpeedMode::UltraFastForward);

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::FastForward);
}

TEST_CASE_METHOD(VrFixture, "desired_speed_mode_ultra_fast_forward_preserved_when_render_throttling_enabled",
    "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::Normal);
    s_cfg.render_throttling = 1;
    g_r4300.desired_speed_mode.store(CoreSpeedMode::UltraFastForward);

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::UltraFastForward);
}

TEST_CASE_METHOD(VrFixture, "normal_makes_normal", "vr_update_effective_speed_mode")
{
    g_r4300.effective_speed_mode.store(CoreSpeedMode::FastForward);
    g_r4300.desired_speed_mode.store(CoreSpeedMode::Normal);

    vr_update_effective_speed_mode();

    REQUIRE(g_r4300.effective_speed_mode.load() == CoreSpeedMode::Normal);
}
