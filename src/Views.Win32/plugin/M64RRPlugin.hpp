/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <plugin/Plugin.hpp>

class M64RRPlugin : public Plugin
{
  public:
    using Plugin::Plugin;
    ~M64RRPlugin() override = default;

    static std::pair<std::string, std::unique_ptr<Plugin>> create(HMODULE module, std::filesystem::path path);
    static std::pair<std::string, std::unique_ptr<Plugin>> create_builtin(Type type, bool dummy = false);

    void config(HWND hwnd) override;
    void test(HWND hwnd) override;
    void about(HWND hwnd) override;
    void initiate(ZESpecFuncs &funcs) override;

  protected:
    FARPROC get_proc(const char *name) const;

    M64RRSpec::PluginMetadata m_meta;
    bool m_initialized{};
    bool m_builtin{};
    std::unordered_map<std::string, FARPROC> m_builtin_procs;
};
