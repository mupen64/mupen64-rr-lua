#pragma once

#include <Windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <windowsx.h>
#include <process.h>
#include <malloc.h>
#include <GL/glew.h>
#include <cmath>
#include <cstdint>
#include <string>
#include <core_plugin.h>
#include <emmintrin.h>
#include <intrin.h>
#include "Types.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define EXPORT __declspec(dllexport)
#define CALL _cdecl
