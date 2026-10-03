/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Types.hpp>

/**
 * \brief The modified control data to be pushed the next frame.
 */
extern CoreButtons g_new_controller_data[4];

/**
 * \brief Whether the modified controller data should be pushed next frame.
 */
extern bool g_overwrite_controller_data[4];

/**
 * \brief Amount of call_input calls.
 */
extern size_t g_input_count;
