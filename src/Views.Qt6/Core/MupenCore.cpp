/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MupenCore.hpp"
#include "Core.hpp"

#include <QUrl>

MupenCore::MupenCore(QObject *parent) : QObject(parent)
{
    Core::context();
}

MupenCore::~MupenCore()
{
}

void MupenCore::vrStartROM(const QUrl &url)
{
    Core::context()->vr_start_rom(url.toLocalFile().toStdU16String());
}

void MupenCore::vrCloseROM(bool resetVCR)
{
    Core::context()->vr_close_rom(resetVCR);
}