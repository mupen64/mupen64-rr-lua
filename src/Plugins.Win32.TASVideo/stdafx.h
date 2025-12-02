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
#include "Types.h"
#include <xbrz.h>
#include <hqx.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define EXPORT __declspec(dllexport)
#define CALL _cdecl
