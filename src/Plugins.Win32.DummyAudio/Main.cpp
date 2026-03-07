/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <DummyPluginStub.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"No Audio", L"1.0.0")

DUMMY_PLUGIN_STUB_IMPL(plugin_audio)