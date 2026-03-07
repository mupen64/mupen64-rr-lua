#pragma once

#include <CommonPCH.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>
#include <Windows.h>
#include <GL/glew.h>
#include <commctrl.h>
#include <windowsx.h>
#include <emmintrin.h>
#include <intrin.h>
#include <xbrz.h>
#include <hqx.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include "Types.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define DEBUG_ERROR 0
#define DEBUG_LOW 0
#define DEBUG_MEDIUM 0
#define DEBUG_HIGH 0
#define DEBUG_HANDLED 0
#define DEBUG_TEXTURE 0
#define DEBUG_COMBINE 0

#define DebugMsg(str, ...)