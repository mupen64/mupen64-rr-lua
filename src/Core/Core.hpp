/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/API.hpp>

extern CoreParams *g_core;
extern CoreCtx g_ctx;
extern std::atomic<int32_t> g_wait_counter;
