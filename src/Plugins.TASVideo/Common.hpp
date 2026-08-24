/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.hpp>
#include <VersionNameHelpers.hpp>
#include <m64rr/API.hpp>
#include <m64rr/Plugin.hpp>
#include <glad/glad.h>
#include <emmintrin.h>
#include <xbrz.h>
#include <hqx.h>
#include <GL/glext.h>
#include "Types.hpp"

#define DEBUG_ERROR 0
#define DEBUG_LOW 0
#define DEBUG_MEDIUM 0
#define DEBUG_HIGH 0
#define DEBUG_HANDLED 0
#define DEBUG_TEXTURE 0
#define DEBUG_COMBINE 0

#define DebugMsg(str, ...)
