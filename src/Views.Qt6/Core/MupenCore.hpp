/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QUrl>
#include <qqmlintegration.h>

class MupenCore : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
  public:
    MupenCore(QObject* parent = nullptr);
    virtual ~MupenCore();

    Q_INVOKABLE void vrStartROM(const QUrl& url);

    Q_INVOKABLE void vrCloseROM(bool resetVCR = true);
};