/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <m64rr/API.hpp>
namespace Core
{
core_cfg &config();
core_params &params();
core_ctx *context();

void clear_plugin_funcs(core_params &params);
} // namespace Core