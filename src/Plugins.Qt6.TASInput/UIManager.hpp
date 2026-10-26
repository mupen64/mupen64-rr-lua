/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <array>
#include <future>

#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>

class TASInputUIManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
public:
    TASInputUIManager(QQuickWindow* main_window, QObject* parent = nullptr);

    virtual ~TASInputUIManager();

    bool isReady();

    void loadComponent();

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
signals:
    void readyChanged();
private:
    Q_INVOKABLE void onStatusUpdate();

    QQuickWindow* m_main_window;
    QQmlComponent* m_component;
    std::array<QQuickWindow*, 4> m_windows;
    bool m_is_open;
};
