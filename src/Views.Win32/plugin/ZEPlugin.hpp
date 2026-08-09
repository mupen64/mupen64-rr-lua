/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <plugin/Plugin.hpp>

class ZEPlugin : public Plugin
{
  public:
    using Plugin::Plugin;
    ~ZEPlugin() override = default;

    static std::pair<std::wstring, std::unique_ptr<Plugin>> create(HMODULE module, std::filesystem::path path);

    void config(HWND hwnd) override;
    void test(HWND hwnd) override;
    void about(HWND hwnd) override;
    void initiate(ZESpecFuncs &funcs) override;
    void initiate_dummy() override;
    void deinitiate_dummy() override;

  protected:
    uint16_t m_version;
};
