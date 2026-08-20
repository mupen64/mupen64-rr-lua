/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QQuickImageProvider>

class QtIconImageProvider : public QQuickImageProvider {
    Q_OBJECT
  public:
    QtIconImageProvider() : QQuickImageProvider(ImageType::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};