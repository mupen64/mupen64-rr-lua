/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <qqmlintegration.h>

class ApplicationExt : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool modalActive READ isModalActive NOTIFY modalWindowChanged)
public:
    explicit ApplicationExt(QObject *parent = nullptr) : QObject(parent) {
        // Force the property to re-evaluate whenever the application's focus shifts
        connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &ApplicationExt::modalActiveChanged);
    }

    bool isModalActive() const {
        return QGuiApplication::modalWindow() != nullptr;
    }

signals:
    void modalActiveChanged();
};
