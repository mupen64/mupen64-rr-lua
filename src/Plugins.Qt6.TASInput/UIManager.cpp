/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "UIManager.hpp"
#include <QQmlComponent>

TASInputUIManager::TASInputUIManager(QQuickWindow *main_window, QObject *parent)
    : QObject(parent), m_main_window(main_window), m_component(nullptr), m_windows{}, m_is_open(false)
{
    auto *engine = qmlEngine(main_window);

    m_component = new QQmlComponent(engine);
    m_component->loadFromModule("TASInput", "MainWindow");
}
TASInputUIManager::~TASInputUIManager()
{
    close();
    delete m_component;
}

void TASInputUIManager::open()
{
    if (m_is_open)
        return;
    m_is_open = true;

    QVariantMap properties {
        {"transientParent", QVariant::fromValue(m_main_window)},
    };
    m_windows[0] = (QQuickWindow*) m_component->createWithInitialProperties(properties);
}

void TASInputUIManager::close() {
    if (!m_is_open)
        return;
    m_is_open = false;
    for (auto *window : m_windows)
    {
        if (window != nullptr)
        {
            delete window;
            window = nullptr;
        }
    }
}
