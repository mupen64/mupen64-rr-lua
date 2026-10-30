/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "UIManager.hpp"

#include "Main.hpp"

#include <QDir>
#include <QQmlComponent>
#include <QUrl>

#define CONFIG_FILE_NAME "TASInputQt6.json"

using namespace BuiltinTAS::TASInput_Qt6;

static QString get_config_path()
{
    const auto size = g_plugin->config_path(nullptr, 0);
    std::string path(size - 1, '\0');
    g_plugin->config_path(path.data(), size);
    return QDir(QString::fromStdString(path)).filePath(CONFIG_FILE_NAME);
}

TASInputUIManager::TASInputUIManager(QQuickWindow *main_window, QObject *parent)
    : QObject(parent), m_main_window(main_window), m_component(nullptr), m_windows{}, m_is_open(false),
      m_settings(new QSettings(get_config_path(), QSettings::IniFormat))
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
    using namespace Qt::Literals;
    if (m_is_open) return;
    m_is_open = true;

    for (size_t i = 0; i < 4; i++) {
        // check if port is present
        bool present = m_settings->value(u"port%1/present"_s.arg(i), i == 0).toBool();
        if (!present)
            continue;

        QVariantMap properties{
            {"transientParent", QVariant::fromValue(m_main_window)},
            {"backend", QVariant::fromValue(this)},
            {"port", QVariant::fromValue((int)i)},
            {"configUrl", QVariant::fromValue(QUrl::fromLocalFile(get_config_path()))},
        };
        m_windows[i] = (QQuickWindow *)m_component->createWithInitialProperties(properties);
        ((QObject *)m_windows[i])->setParent(this);
    }


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

bool TASInputUIManager::hasPort(int port)
{
    return m_windows[port] != nullptr;
}

void TASInputUIManager::onStatusUpdate()
{
    qInfo() << "status: " << m_component->status();
    if (m_component->status() == QQmlComponent::Error)
    {
        qInfo() << "error: " << m_component->errorString();
    }
    if (m_component->status() == QQmlComponent::Ready) readyChanged();
}
