/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "m64rr/Types.hpp"

namespace DialogService
{
int show_multiple_choice_dialog(std::string_view id, const std::vector<std::string> &choices, const char *str,
                                const char *title, core_dialog_type type);

bool show_ask_dialog(std::string_view id, const char *str, const char *title, bool warning);

void show_dialog(const char *str, const char *title, core_dialog_type type);
} // namespace DialogService