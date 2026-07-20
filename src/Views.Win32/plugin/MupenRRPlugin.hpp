/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <plugin/Plugin.hpp>

class MupenRRPlugin : public Plugin
{
  public:
    using Plugin::Plugin;
    ~MupenRRPlugin() override = default;

    void config(HWND hwnd) override;
    void test(HWND hwnd) override;
    void about(HWND hwnd) override;
    void initiate() override;
    void initiate_dummy() override;
    void deinitiate_dummy() override;
};
