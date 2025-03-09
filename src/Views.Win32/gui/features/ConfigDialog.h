/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void configdialog_init();
void configdialog_show();
void configdialog_about();
bool configdialog_show_plugin(Plugin* plugin, core_plugin_cfg* cfg);