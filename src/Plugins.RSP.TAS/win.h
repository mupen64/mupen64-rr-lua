/*
 * Copyright (c) 2025, hacktarux-azimer-rsp-hle maintainers, contributors, and original authors (Hacktarux, Azimer).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/stdafx.h>
#include <core_api.h>
#include <core_plugin.h>
#include <resource.h>

extern HINSTANCE g_instance;
extern std::filesystem::path g_app_path;
extern PlatformService g_platform_service;

INT_PTR CALLBACK ConfigDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
