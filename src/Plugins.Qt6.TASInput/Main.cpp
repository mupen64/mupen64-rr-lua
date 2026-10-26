/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include <Core/Plugin.hpp>
#include <QQuickWindow>

#include "UIManager.hpp"

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("TASInput (Qt6)")
namespace BuiltinTAS::TASInput_Qt6
{

M64RRSpec::PluginInit *g_plugin = nullptr;
static std::atomic<TASInputUIManager *> g_ui_manager = nullptr;

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::Audio;

    constexpr auto name = PLUGIN_NAME;
    constexpr auto description = "Built-in plugin for Mupen64."
                                 "\n\n"
                                 "https://mupen64.com";

    auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);
    metadata->name[result.size] = '\0';

    result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);
    metadata->description[result.size] = '\0';
}

EXPORT void CALL M64RRProcessEvent(M64RRSpec::Event event)
{
    switch (event.type)
    {
    case M64RRSpec::Event::Type::Initiate: {
        g_plugin = event.initiate.init;
        assert(g_plugin->platform == M64RRSpec::Platform::Qt6);

        if (g_ui_manager != nullptr)
            break;

        // create the UI manager
        auto *window = g_plugin->main_window.qt_window();
        QMetaObject::invokeMethod(window, [window] {
            auto *ui_manager = new TASInputUIManager(window);

            QObject::connect(ui_manager, &TASInputUIManager::readyChanged, ui_manager, [=] {
                if (!ui_manager->isReady())
                    return;
                std::println("ready!");
                g_ui_manager.store(ui_manager, std::memory_order_release);
                g_ui_manager.notify_all();
            }, Qt::SingleShotConnection);
            ui_manager->loadComponent();

        });
        break;
    }
    case M64RRSpec::Event::Type::InitializeControllers: {
        auto *controllers = event.initialize_controllers.controllers;

        for (int i = 0; i < 4; ++i)
        {
            controllers[i].present = 0;
            controllers[i].raw = 0;
            controllers[i].plugin = CoreControllerExtension::None;
        }

        controllers[0].present = 1;
        break;
    }
    case M64RRSpec::Event::Type::RomOpened: {
        TASInputUIManager* ui_manager = nullptr;
        // wait until g_ui_manager is set
        while ((ui_manager = g_ui_manager.load(std::memory_order_acquire)) == nullptr)
        {
            g_ui_manager.wait(nullptr, std::memory_order_relaxed);
        }

        QMetaObject::invokeMethod(ui_manager, &TASInputUIManager::open);
        break;
    }
    case M64RRSpec::Event::Type::RomClosed: {
        auto *ui_manager = g_ui_manager.load();
        QMetaObject::invokeMethod(ui_manager, &TASInputUIManager::close);
        break;
    }
    case M64RRSpec::Event::Type::Shutdown: {
        auto *window = g_plugin->main_window.qt_window();
        QMetaObject::invokeMethod(window, [] {
            delete g_ui_manager.load();
            g_ui_manager = nullptr;
        });
        break;
    }
    default:
        break;
    }
}

void M64RRReadController(int32_t controller, unsigned char *command)
{
}
void M64RRShowConfig(M64RRSpec::WindowHandle parent_window)
{
}
void M64RRGetKeys(int32_t index, CoreButtons *buttons)
{
    buttons->value = 0;
}
void M64RRSetKeys(int32_t index, CoreButtons buttons)
{
}
} // namespace BuiltinTAS::TASInput_Qt6
