/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MupenCore.hpp"
#include <ViewModels.Qt6/Core.hpp>

#include <QUrl>

MupenCore::MupenCore(QObject *parent) : QObject(parent)
{
    Core::context();
}

MupenCore::~MupenCore()
{
}

CoreResult::Value MupenCore::vrStartROM(const QUrl &url)
{
    return (CoreResult::Value)(int) Core::context()->vr_start_rom(url.toLocalFile().toStdU16String());
}

CoreResult::Value MupenCore::vrCloseROM(bool resetVCR)
{
    return (CoreResult::Value)(int) Core::context()->vr_close_rom(resetVCR);
}