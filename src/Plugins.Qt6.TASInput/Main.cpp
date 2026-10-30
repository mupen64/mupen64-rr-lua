/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include <Core/Plugin.hpp>
#include <QQuickWindow>
#include <QTimer>

#include "UIManager.hpp"

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("TASInput (Qt6)")
namespace BuiltinTAS::TASInput_Qt6
{

M64RRSpec::PluginInit *g_plugin = nullptr;

// managed by the UI thread. DO NOT MODIFY from outside.
QPointer<TASInputUIManager> g_ui_manager = nullptr;
// managed by the plugin. DO NOT MODIFY from outside.
QPointer<TASInputUIManager> g_ext_ui_manager = nullptr;

namespace
{
CoreButtons jsObjectToButtons(const QVariantMap &map)
{
    return {
        .dr = map["btnDR"].toBool(),
        .dl = map["btnDL"].toBool(),
        .dd = map["btnDD"].toBool(),
        .du = map["btnDU"].toBool(),
        .start = map["btnS"].toBool(),
        .z = map["btnZ"].toBool(),
        .b = map["btnB"].toBool(),
        .a = map["btnA"].toBool(),
        .cr = map["btnCR"].toBool(),
        .cl = map["btnCL"].toBool(),
        .cd = map["btnCD"].toBool(),
        .cu = map["btnCU"].toBool(),
        .r = map["btnR"].toBool(),
        .l = map["btnL"].toBool(),
        .reserved_1 = 0,
        .reserved_2 = 0,
        .x = map["x"].toInt(),
        .y = map["y"].toInt(),
    };
}
QVariantMap buttonsToJSObject(CoreButtons buttons)
{
    return QVariantMap{
        {"btnDR", QVariant::fromValue(buttons.dr)},
        {"btnDL", QVariant::fromValue(buttons.dl)},
        {"btnDD", QVariant::fromValue(buttons.dd)},
        {"btnDU", QVariant::fromValue(buttons.du)},
        {"btnS", QVariant::fromValue(buttons.start)},
        {"btnZ", QVariant::fromValue(buttons.z)},
        {"btnB", QVariant::fromValue(buttons.b)},
        {"btnA", QVariant::fromValue(buttons.a)},
        {"btnCR", QVariant::fromValue(buttons.cr)},
        {"btnCL", QVariant::fromValue(buttons.cl)},
        {"btnCD", QVariant::fromValue(buttons.cd)},
        {"btnCU", QVariant::fromValue(buttons.cu)},
        {"btnR", QVariant::fromValue(buttons.r)},
        {"btnL", QVariant::fromValue(buttons.l)},
        {"x", QVariant::fromValue(buttons.x)},
        {"y", QVariant::fromValue(buttons.y)},
    };
}
} // namespace

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::Input;

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
        auto *window = g_plugin->main_window.qt_window();

        // BlockingQueuedConnection isn't sufficient, as we may have to wait for a second event
        // for our "coroutine" to complete.
        std::atomic<bool> done_flag = false;
        std::atomic<TASInputUIManager *> ui_manager = nullptr;

        // We don't have coroutines, so this looks a bit funky.
        QMetaObject::invokeMethod(window, [window, &done_flag, &ui_manager] mutable {
            if (!g_ui_manager) g_ui_manager = new TASInputUIManager(window);
            ui_manager.store(g_ui_manager);

            auto open_windows = [&done_flag] mutable {
                g_ui_manager->open();
                done_flag = true;
                done_flag.notify_all();
            };

            // open the windows when the component has loaded (either now or later)
            if (g_ui_manager->isReady())
            {
                open_windows();
                return;
            }
            QObject::connect(
                g_ui_manager, &TASInputUIManager::readyChanged, g_ui_manager, open_windows, Qt::SingleShotConnection);
            g_ui_manager->loadComponent();
        });

        // wait until the windows are open
        while (!done_flag.load()) done_flag.wait(false);

        // ui_manager should have been previously set by the UI thread
        g_ext_ui_manager = ui_manager.load();

        break;
    }
    case M64RRSpec::Event::Type::RomClosed: {
        if (g_ext_ui_manager.isNull()) break;
        QMetaObject::invokeMethod(g_ext_ui_manager.get(), &TASInputUIManager::close);
        break;
    }
    case M64RRSpec::Event::Type::Shutdown: {
        if (g_ext_ui_manager.isNull()) break;
        auto *window = g_plugin->main_window.qt_window();
        QMetaObject::invokeMethod(window, [] {
            delete g_ui_manager;
            g_ui_manager = nullptr;
        }, Qt::QueuedConnection);
        break;
    }
    default:
        break;
    }
}

void M64RRReadController(int32_t controller, unsigned char *command)
{
}
void M64RRGetKeys(int32_t index, CoreButtons *buttons)
{
    if (g_ext_ui_manager.isNull()) return;

    // Temporary event loop to wait for a signal
    QEventLoop event_loop;
    QTimer timeout;

    // once the input arrives, shut down the event loop and set the buttons
    QObject::connect(g_ext_ui_manager.get(), &TASInputUIManager::receiveInput, &event_loop,
        [=, &event_loop](int port, const QVariantMap &map) {
            if (port != index) return;
            *buttons = jsObjectToButtons(map);
            event_loop.quit();
        });

    // stop if input polling takes too long
    QObject::connect(&timeout, &QTimer::timeout, &event_loop, &QEventLoop::quit);

    // fire the start signal; the event loop will be stopped by the handler
    timeout.start(50);
    QMetaObject::invokeMethod(g_ext_ui_manager.get(), &TASInputUIManager::requestInput, index);
    event_loop.exec();
}
void M64RRSetKeys(int32_t index, CoreButtons buttons)
{
    // block until the signal fires
    QMetaObject::invokeMethod(g_ext_ui_manager, &TASInputUIManager::replaceInput, Qt::BlockingQueuedConnection, index,
        buttonsToJSObject(buttons));
}
} // namespace BuiltinTAS::TASInput_Qt6
