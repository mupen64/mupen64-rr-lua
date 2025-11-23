/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEW_UTILS_HPP_INCLUDED
#define VIEW_UTILS_HPP_INCLUDED

#include <QString>
#include <QStringView>

inline QString str_to_qstring(std::string_view str) {
  return QString::fromUtf8(str.data(), str.size());
}

#endif