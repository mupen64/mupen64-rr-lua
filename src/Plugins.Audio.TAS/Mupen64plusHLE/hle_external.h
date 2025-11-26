/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/* users of the hle core are expected to define these functions */

void HleVerboseMessage(void* user_defined, const char* message, ...);
void HleErrorMessage(void* user_defined, const char* message, ...);
void HleWarnMessage(void* user_defined, const char* message, ...);

void HleCheckInterrupts(void* user_defined);
void HleProcessDlistList(void* user_defined);
void HleProcessAlistList(void* user_defined);
void HleProcessRdpList(void* user_defined);
void HleShowCFB(void* user_defined);
