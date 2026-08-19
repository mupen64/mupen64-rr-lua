/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QUrl>
#include <qqmlintegration.h>

#include "CoreEnums.hpp"

class MupenCore : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
  public:
    MupenCore(QObject* parent = nullptr);
    virtual ~MupenCore();


    // vr_* functions
    // ==========================

    Q_INVOKABLE CoreResult::Value vrStartROM(const QUrl& url);

    Q_INVOKABLE CoreResult::Value vrCloseROM(bool resetVCR = true);

};