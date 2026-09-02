/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <qqmlintegration.h>

class PathsUtil : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Paths)
    QML_SINGLETON
  public:
    Q_INVOKABLE QVariant toLocalFile(QUrl url)
    {
        return (url.isLocalFile()) ? QVariant(url.toLocalFile()) : QVariant();
    }
};
