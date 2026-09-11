/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QJSEngine>
#include <QJSValue>
#include <qqmlintegration.h>

/**
 * Global object for tracking key releases for HeldShortcut.
 */
class HeldShortcutMap : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
  public:
    HeldShortcutMap(QObject* parent = nullptr);
    virtual ~HeldShortcutMap();

    // Binds a specific key to the provided object.
    Q_INVOKABLE bool addShortcut(Qt::Key key, QObject* object);
    // Clears all keys bound to this object.
    Q_INVOKABLE void clearShortcuts(QObject* object);

    bool eventFilter(QObject *watched, QEvent *event) override;
  private:
    std::unordered_map<Qt::Key, QObject*> m_shortcuts;
};

Q_DECLARE_METATYPE(HeldShortcutMap)
