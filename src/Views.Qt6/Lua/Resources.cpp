/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "Resources.hpp"

#include <cassert>

#include <QResource>
#include <cstddef>
#include <string_view>

namespace LuaResources
{
static std::string_view load(const QString &path)
{
    const QResource res(path);
    assert(res.isValid());
    assert(res.compressionAlgorithm() == QResource::NoCompression);
    return {reinterpret_cast<const char *>(res.data()), static_cast<size_t>(res.size())};
}

std::string_view sandbox()
{
    return load(QStringLiteral(":/lua/sandbox.lua"));
}

std::string_view api()
{
    return load(QStringLiteral(":/lua/api.lua"));
}

std::string_view inspect()
{
    return load(QStringLiteral(":/lua/inspect.lua"));
}
} // namespace LuaResources
