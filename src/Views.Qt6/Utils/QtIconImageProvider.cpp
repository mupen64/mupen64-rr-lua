/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtIconImageProvider.hpp>

#include <QIcon>

QPixmap QtIconImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    auto icon = QIcon::fromTheme(id);

    QSize actual_size = requestedSize.isValid() ? requestedSize : QSize(64, 64);
    *size = actual_size;
    return icon.pixmap(actual_size);
}