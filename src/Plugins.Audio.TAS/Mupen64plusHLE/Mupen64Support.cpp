/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../common.h"
#include "hle_external.h"
#include "hle_internal.h"

static hle_t hle;

void HleWarnMessage(void* user_defined, const char* message, ...)
{
}

void HleVerboseMessage(void* user_defined, const char* message, ...)
{
}

void SetupMusyX()
{
    hle.dram = AudioInfo.rdram;
    hle.dmem = AudioInfo.dmem;
    hle.imem = AudioInfo.imem;
}

void ProcessMusyX_v1()
{
    SetupMusyX();
    musyx_v1_task(&hle);
}

void ProcessMusyX_v2()
{
    SetupMusyX();
    musyx_v2_task(&hle);
}
