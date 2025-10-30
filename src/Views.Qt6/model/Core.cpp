/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Core.hpp"

namespace {
core_params core_params_init() {
    return core_params {
        .callbacks = core_callbacks {},
    };
}
}

namespace Mupen
{
}