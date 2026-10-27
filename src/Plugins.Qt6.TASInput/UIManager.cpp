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

    m_component = new QQmlComponent(engine, this);
    // The component might not be immediately ready. This will set off a signal when it is.
    QObject::connect(m_component, &QQmlComponent::statusChanged, this, &TASInputUIManager::onStatusUpdate);
}
TASInputUIManager::~TASInputUIManager()
{
}

bool TASInputUIManager::isReady()
{
    return m_component->isReady();
}

void TASInputUIManager::loadComponent()
{
    m_component->loadFromModule("TASInput", "MainWindow");
}

void TASInputUIManager::open()
{
    if (m_is_open) return;
    m_is_open = true;

    QVariantMap properties{
        {"transientParent", QVariant::fromValue(m_main_window)},
        {"backend", QVariant::fromValue(this)},
        {"port", 0},
    };
    m_windows[0] = (QQuickWindow *)m_component->createWithInitialProperties(properties);
}

void TASInputUIManager::close()
{
    if (!m_is_open) return;
    m_is_open = false;
    for (auto *&window : m_windows)
    {
        if (window != nullptr)
        {
            delete window;
            window = nullptr;
        }
    }
}

bool TASInputUIManager::hasPort(int port) {
    return m_windows[port] != nullptr;
}

void TASInputUIManager::onStatusUpdate()
{
    qInfo() << "status: " << m_component->status();
    if (m_component->status() == QQmlComponent::Error) {
        qInfo() << "error: " << m_component->errorString();
    }
    readyChanged();
}
