/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>

class TASInputUIManager : public QObject {
    Q_OBJECT
public:
    TASInputUIManager(QQuickWindow* main_window, QObject* parent = nullptr);

    virtual ~TASInputUIManager();

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
private:
    QQuickWindow* m_main_window;
    QQmlComponent* m_component;
    std::array<QQuickWindow*, 4> m_windows;
    bool m_is_open;
};
