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

class CoreContext : public QObject {
    Q_OBJECT
    QML_ELEMENT
  public:
    CoreContext(QObject* parent = nullptr);
    virtual ~CoreContext();

    // vr_* functions
    // ==========================

    Q_INVOKABLE CoreResult::Value vrStartROM(const QUrl& url);

    Q_INVOKABLE CoreResult::Value vrCloseROM(bool resetVCR = true);

  signals:
    // Dialog service
    // ==========================

    size_t showMultipleChoiceDialog(QAnyStringView title, QAnyStringView content, const QList<QString>& choices, CoreDialogType::Value type);

    bool showAskDialog(QAnyStringView title, QAnyStringView content, CoreDialogType::Value type);

    void showDialog(QAnyStringView title, QAnyStringView content, CoreDialogType::Value type);

};