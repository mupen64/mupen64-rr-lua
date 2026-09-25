/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QUrl>
#include <qqmlintegration.h>

class DefaultPaths : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
  public:
    DefaultPaths(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString romDir();

    Q_INVOKABLE QString saveDir();

    Q_INVOKABLE QString screenshotDir();

    Q_INVOKABLE QString backupDir();
};
