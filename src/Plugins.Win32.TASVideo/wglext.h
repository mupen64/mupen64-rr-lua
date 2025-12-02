#ifndef __wglext_h_
#define __wglext_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: This software was created using the
** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
** not been independently verified as being compliant with the OpenGL(R)
** version 1.2.1 Specification.
*/

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

/*************************************************************/

/* Header file version number */
/* wglext.h last updated 2002/03/22 */
/* Current version at http://oss.sgi.com/projects/ogl-sample/registry/ */
#define WGL_WGLEXT_VERSION 4

#ifndef WGL_ARB_buffer_region
#define WGL_FRONT_COLOR_BUFFER_BIT_ARB 0x00000001
#define WGL_BACK_COLOR_BUFFER_BIT_ARB 0x00000002
#define WGL_DEPTH_BUFFER_BIT_ARB 0x00000004
#define WGL_STENCIL_BUFFER_BIT_ARB 0x00000008
#endif

#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#endif

#ifndef WGL_ARB_extensions_string
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#endif

#ifndef WGL_ARB_make_current_read
#define ERROR_INVALID_PIXEL_TYPE_ARB 0x2043
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054
#endif

#ifndef WGL_ARB_pbuffer
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB 0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB 0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB 0x2030
#define WGL_PBUFFER_LARGEST_ARB 0x2033
#define WGL_PBUFFER_WIDTH_ARB 0x2034
#define WGL_PBUFFER_HEIGHT_ARB 0x2035
#define WGL_PBUFFER_LOST_ARB 0x2036
#endif

#ifndef WGL_ARB_render_texture
#define WGL_BIND_TO_TEXTURE_RGB_ARB 0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB 0x2071
#define WGL_TEXTURE_FORMAT_ARB 0x2072
#define WGL_TEXTURE_TARGET_ARB 0x2073
#define WGL_MIPMAP_TEXTURE_ARB 0x2074
#define WGL_TEXTURE_RGB_ARB 0x2075
#define WGL_TEXTURE_RGBA_ARB 0x2076
#define WGL_NO_TEXTURE_ARB 0x2077
#define WGL_TEXTURE_CUBE_MAP_ARB 0x2078
#define WGL_TEXTURE_1D_ARB 0x2079
#define WGL_TEXTURE_2D_ARB 0x207A
#define WGL_MIPMAP_LEVEL_ARB 0x207B
#define WGL_CUBE_MAP_FACE_ARB 0x207C
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x207E
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x207F
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x2080
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x2081
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x2082
#define WGL_FRONT_LEFT_ARB 0x2083
#define WGL_FRONT_RIGHT_ARB 0x2084
#define WGL_BACK_LEFT_ARB 0x2085
#define WGL_BACK_RIGHT_ARB 0x2086
#define WGL_AUX0_ARB 0x2087
#define WGL_AUX1_ARB 0x2088
#define WGL_AUX2_ARB 0x2089
#define WGL_AUX3_ARB 0x208A
#define WGL_AUX4_ARB 0x208B
#define WGL_AUX5_ARB 0x208C
#define WGL_AUX6_ARB 0x208D
#define WGL_AUX7_ARB 0x208E
#define WGL_AUX8_ARB 0x208F
#define WGL_AUX9_ARB 0x2090
#endif

#ifndef WGL_EXT_make_current_read
#define ERROR_INVALID_PIXEL_TYPE_EXT 0x2043
#endif

#ifndef WGL_EXT_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_EXT 0x2000
#define WGL_DRAW_TO_WINDOW_EXT 0x2001
#define WGL_DRAW_TO_BITMAP_EXT 0x2002
#define WGL_ACCELERATION_EXT 0x2003
#define WGL_NEED_PALETTE_EXT 0x2004
#define WGL_NEED_SYSTEM_PALETTE_EXT 0x2005
#define WGL_SWAP_LAYER_BUFFERS_EXT 0x2006
#define WGL_SWAP_METHOD_EXT 0x2007
#define WGL_NUMBER_OVERLAYS_EXT 0x2008
#define WGL_NUMBER_UNDERLAYS_EXT 0x2009
#define WGL_TRANSPARENT_EXT 0x200A
#define WGL_TRANSPARENT_VALUE_EXT 0x200B
#define WGL_SHARE_DEPTH_EXT 0x200C
#define WGL_SHARE_STENCIL_EXT 0x200D
#define WGL_SHARE_ACCUM_EXT 0x200E
#define WGL_SUPPORT_GDI_EXT 0x200F
#define WGL_SUPPORT_OPENGL_EXT 0x2010
#define WGL_DOUBLE_BUFFER_EXT 0x2011
#define WGL_STEREO_EXT 0x2012
#define WGL_PIXEL_TYPE_EXT 0x2013
#define WGL_COLOR_BITS_EXT 0x2014
#define WGL_RED_BITS_EXT 0x2015
#define WGL_RED_SHIFT_EXT 0x2016
#define WGL_GREEN_BITS_EXT 0x2017
#define WGL_GREEN_SHIFT_EXT 0x2018
#define WGL_BLUE_BITS_EXT 0x2019
#define WGL_BLUE_SHIFT_EXT 0x201A
#define WGL_ALPHA_BITS_EXT 0x201B
#define WGL_ALPHA_SHIFT_EXT 0x201C
#define WGL_ACCUM_BITS_EXT 0x201D
#define WGL_ACCUM_RED_BITS_EXT 0x201E
#define WGL_ACCUM_GREEN_BITS_EXT 0x201F
#define WGL_ACCUM_BLUE_BITS_EXT 0x2020
#define WGL_ACCUM_ALPHA_BITS_EXT 0x2021
#define WGL_DEPTH_BITS_EXT 0x2022
#define WGL_STENCIL_BITS_EXT 0x2023
#define WGL_AUX_BUFFERS_EXT 0x2024
#define WGL_NO_ACCELERATION_EXT 0x2025
#define WGL_GENERIC_ACCELERATION_EXT 0x2026
#define WGL_FULL_ACCELERATION_EXT 0x2027
#define WGL_SWAP_EXCHANGE_EXT 0x2028
#define WGL_SWAP_COPY_EXT 0x2029
#define WGL_SWAP_UNDEFINED_EXT 0x202A
#define WGL_TYPE_RGBA_EXT 0x202B
#define WGL_TYPE_COLORINDEX_EXT 0x202C
#endif

#ifndef WGL_EXT_pbuffer
#define WGL_DRAW_TO_PBUFFER_EXT 0x202D
#define WGL_MAX_PBUFFER_PIXELS_EXT 0x202E
#define WGL_MAX_PBUFFER_WIDTH_EXT 0x202F
#define WGL_MAX_PBUFFER_HEIGHT_EXT 0x2030
#define WGL_OPTIMAL_PBUFFER_WIDTH_EXT 0x2031
#define WGL_OPTIMAL_PBUFFER_HEIGHT_EXT 0x2032
#define WGL_PBUFFER_LARGEST_EXT 0x2033
#define WGL_PBUFFER_WIDTH_EXT 0x2034
#define WGL_PBUFFER_HEIGHT_EXT 0x2035
#endif

#ifndef WGL_EXT_depth_float
#define WGL_DEPTH_FLOAT_EXT 0x2040
#endif

#ifndef WGL_3DFX_multisample
#define WGL_SAMPLE_BUFFERS_3DFX 0x2060
#define WGL_SAMPLES_3DFX 0x2061
#endif

#ifndef WGL_EXT_multisample
#define WGL_SAMPLE_BUFFERS_EXT 0x2041
#define WGL_SAMPLES_EXT 0x2042
#endif

#ifndef WGL_I3D_digital_video_control
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_FRAMEBUFFER_I3D 0x2050
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_VALUE_I3D 0x2051
#define WGL_DIGITAL_VIDEO_CURSOR_INCLUDED_I3D 0x2052
#define WGL_DIGITAL_VIDEO_GAMMA_CORRECTED_I3D 0x2053
#endif

#ifndef WGL_I3D_gamma
#define WGL_GAMMA_TABLE_SIZE_I3D 0x204E
#define WGL_GAMMA_EXCLUDE_DESKTOP_I3D 0x204F
#endif

#ifndef WGL_I3D_genlock
#define WGL_GENLOCK_SOURCE_MULTIVIEW_I3D 0x2044
#define WGL_GENLOCK_SOURCE_EXTENAL_SYNC_I3D 0x2045
#define WGL_GENLOCK_SOURCE_EXTENAL_FIELD_I3D 0x2046
#define WGL_GENLOCK_SOURCE_EXTENAL_TTL_I3D 0x2047
#define WGL_GENLOCK_SOURCE_DIGITAL_SYNC_I3D 0x2048
#define WGL_GENLOCK_SOURCE_DIGITAL_FIELD_I3D 0x2049
#define WGL_GENLOCK_SOURCE_EDGE_FALLING_I3D 0x204A
#define WGL_GENLOCK_SOURCE_EDGE_RISING_I3D 0x204B
#define WGL_GENLOCK_SOURCE_EDGE_BOTH_I3D 0x204C
#endif

#ifndef WGL_I3D_image_buffer
#define WGL_IMAGE_BUFFER_MIN_ACCESS_I3D 0x00000001
#define WGL_IMAGE_BUFFER_LOCK_I3D 0x00000002
#endif

#ifndef WGL_I3D_swap_frame_lock
#endif


/*************************************************************/

#ifndef WGL_ARB_pbuffer
DECLARE_HANDLE(HPBUFFERARB);
#endif
#ifndef WGL_EXT_pbuffer
DECLARE_HANDLE(HPBUFFEREXT);
#endif

#ifndef WGL_ARB_buffer_region
#define WGL_ARB_buffer_region 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HANDLE WINAPI wglCreateBufferRegionARB(HDC, int, UINT);
extern VOID WINAPI wglDeleteBufferRegionARB(HANDLE);
extern BOOL WINAPI wglSaveBufferRegionARB(HANDLE, int, int, int, int);
extern BOOL WINAPI wglRestoreBufferRegionARB(HANDLE, int, int, int, int, int, int);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLCREATEBUFFERREGIONARBPROC = HANDLE(WINAPI*)(HDC hDC, int iLayerPlane, UINT uType);
using PFNWGLDELETEBUFFERREGIONARBPROC = VOID(WINAPI*)(HANDLE hRegion);
using PFNWGLSAVEBUFFERREGIONARBPROC = BOOL(WINAPI*)(HANDLE hRegion, int x, int y, int width, int height);
using PFNWGLRESTOREBUFFERREGIONARBPROC = BOOL(WINAPI*)(HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
#endif

#ifndef WGL_ARB_multisample
#define WGL_ARB_multisample 1
#endif

#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern const char* WINAPI wglGetExtensionsStringARB(HDC);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETEXTENSIONSSTRINGARBPROC = const char*(WINAPI*)(HDC hdc);
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetPixelFormatAttribivARB(HDC, int, int, UINT, const int*, int*);
extern BOOL WINAPI wglGetPixelFormatAttribfvARB(HDC, int, int, UINT, const int*, FLOAT*);
extern BOOL WINAPI wglChoosePixelFormatARB(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETPIXELFORMATATTRIBIVARBPROC = BOOL(WINAPI*)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, int* piValues);
using PFNWGLGETPIXELFORMATATTRIBFVARBPROC = BOOL(WINAPI*)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, FLOAT* pfValues);
using PFNWGLCHOOSEPIXELFORMATARBPROC = BOOL(WINAPI*)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
#endif

#ifndef WGL_ARB_make_current_read
#define WGL_ARB_make_current_read 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglMakeContextCurrentARB(HDC, HDC, HGLRC);
extern HDC WINAPI wglGetCurrentReadDCARB(void);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLMAKECONTEXTCURRENTARBPROC = BOOL(WINAPI*)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
using PFNWGLGETCURRENTREADDCARBPROC = HDC(WINAPI*)(void);
#endif

#ifndef WGL_ARB_pbuffer
#define WGL_ARB_pbuffer 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HPBUFFERARB WINAPI wglCreatePbufferARB(HDC, int, int, int, const int*);
extern HDC WINAPI wglGetPbufferDCARB(HPBUFFERARB);
extern int WINAPI wglReleasePbufferDCARB(HPBUFFERARB, HDC);
extern BOOL WINAPI wglDestroyPbufferARB(HPBUFFERARB);
extern BOOL WINAPI wglQueryPbufferARB(HPBUFFERARB, int, int*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLCREATEPBUFFERARBPROC = HPBUFFERARB(WINAPI*)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
using PFNWGLGETPBUFFERDCARBPROC = HDC(WINAPI*)(HPBUFFERARB hPbuffer);
using PFNWGLRELEASEPBUFFERDCARBPROC = int(WINAPI*)(HPBUFFERARB hPbuffer, HDC hDC);
using PFNWGLDESTROYPBUFFERARBPROC = BOOL(WINAPI*)(HPBUFFERARB hPbuffer);
using PFNWGLQUERYPBUFFERARBPROC = BOOL(WINAPI*)(HPBUFFERARB hPbuffer, int iAttribute, int* piValue);
#endif

#ifndef WGL_ARB_render_texture
#define WGL_ARB_render_texture 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglBindTexImageARB(HPBUFFERARB, int);
extern BOOL WINAPI wglReleaseTexImageARB(HPBUFFERARB, int);
extern BOOL WINAPI wglSetPbufferAttribARB(HPBUFFERARB, const int*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLBINDTEXIMAGEARBPROC = BOOL(WINAPI*)(HPBUFFERARB hPbuffer, int iBuffer);
using PFNWGLRELEASETEXIMAGEARBPROC = BOOL(WINAPI*)(HPBUFFERARB hPbuffer, int iBuffer);
using PFNWGLSETPBUFFERATTRIBARBPROC = BOOL(WINAPI*)(HPBUFFERARB hPbuffer, const int* piAttribList);
#endif

#ifndef WGL_EXT_display_color_table
#define WGL_EXT_display_color_table 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern GLboolean WINAPI wglCreateDisplayColorTableEXT(GLushort);
extern GLboolean WINAPI wglLoadDisplayColorTableEXT(const GLushort*, GLuint);
extern GLboolean WINAPI wglBindDisplayColorTableEXT(GLushort);
extern VOID WINAPI wglDestroyDisplayColorTableEXT(GLushort);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLCREATEDISPLAYCOLORTABLEEXTPROC = GLboolean(WINAPI*)(GLushort id);
using PFNWGLLOADDISPLAYCOLORTABLEEXTPROC = GLboolean(WINAPI*)(const GLushort* table, GLuint length);
using PFNWGLBINDDISPLAYCOLORTABLEEXTPROC = GLboolean(WINAPI*)(GLushort id);
using PFNWGLDESTROYDISPLAYCOLORTABLEEXTPROC = VOID(WINAPI*)(GLushort id);
#endif

#ifndef WGL_EXT_extensions_string
#define WGL_EXT_extensions_string 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern const char* WINAPI wglGetExtensionsStringEXT(void);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETEXTENSIONSSTRINGEXTPROC = const char*(WINAPI*)(void);
#endif

#ifndef WGL_EXT_make_current_read
#define WGL_EXT_make_current_read 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglMakeContextCurrentEXT(HDC, HDC, HGLRC);
extern HDC WINAPI wglGetCurrentReadDCEXT(void);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLMAKECONTEXTCURRENTEXTPROC = BOOL(WINAPI*)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
using PFNWGLGETCURRENTREADDCEXTPROC = HDC(WINAPI*)(void);
#endif

#ifndef WGL_EXT_pbuffer
#define WGL_EXT_pbuffer 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HPBUFFEREXT WINAPI wglCreatePbufferEXT(HDC, int, int, int, const int*);
extern HDC WINAPI wglGetPbufferDCEXT(HPBUFFEREXT);
extern int WINAPI wglReleasePbufferDCEXT(HPBUFFEREXT, HDC);
extern BOOL WINAPI wglDestroyPbufferEXT(HPBUFFEREXT);
extern BOOL WINAPI wglQueryPbufferEXT(HPBUFFEREXT, int, int*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLCREATEPBUFFEREXTPROC = HPBUFFEREXT(WINAPI*)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
using PFNWGLGETPBUFFERDCEXTPROC = HDC(WINAPI*)(HPBUFFEREXT hPbuffer);
using PFNWGLRELEASEPBUFFERDCEXTPROC = int(WINAPI*)(HPBUFFEREXT hPbuffer, HDC hDC);
using PFNWGLDESTROYPBUFFEREXTPROC = BOOL(WINAPI*)(HPBUFFEREXT hPbuffer);
using PFNWGLQUERYPBUFFEREXTPROC = BOOL(WINAPI*)(HPBUFFEREXT hPbuffer, int iAttribute, int* piValue);
#endif

#ifndef WGL_EXT_pixel_format
#define WGL_EXT_pixel_format 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetPixelFormatAttribivEXT(HDC, int, int, UINT, int*, int*);
extern BOOL WINAPI wglGetPixelFormatAttribfvEXT(HDC, int, int, UINT, int*, FLOAT*);
extern BOOL WINAPI wglChoosePixelFormatEXT(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETPIXELFORMATATTRIBIVEXTPROC = BOOL(WINAPI*)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int* piAttributes, int* piValues);
using PFNWGLGETPIXELFORMATATTRIBFVEXTPROC = BOOL(WINAPI*)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int* piAttributes, FLOAT* pfValues);
using PFNWGLCHOOSEPIXELFORMATARBPROC = BOOL(WINAPI*)(HDC hDC, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);

#endif

#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglSwapIntervalEXT(int);
extern int WINAPI wglGetSwapIntervalEXT(void);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLSWAPINTERVALEXTPROC = BOOL(WINAPI*)(int interval);
using PFNWGLGETSWAPINTERVALEXTPROC = int(WINAPI*)(void);
#endif

#ifndef WGL_EXT_depth_float
#define WGL_EXT_depth_float 1
#endif

#ifndef WGL_NV_vertex_array_range
#define WGL_NV_vertex_array_range 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern void* WINAPI wglAllocateMemoryNV(GLsizei, GLfloat, GLfloat, GLfloat);
extern void WINAPI wglFreeMemoryNV(void*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLALLOCATEMEMORYNVPROC = void*(WINAPI*)(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
using PFNWGLFREEMEMORYNVPROC = void(WINAPI*)(void* pointer);
#endif

#ifndef WGL_3DFX_multisample
#define WGL_3DFX_multisample 1
#endif

#ifndef WGL_EXT_multisample
#define WGL_EXT_multisample 1
#endif

#ifndef WGL_OML_sync_control
#define WGL_OML_sync_control 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetSyncValuesOML(HDC, INT64*, INT64*, INT64*);
extern BOOL WINAPI wglGetMscRateOML(HDC, INT32*, INT32*);
extern INT64 WINAPI wglSwapBuffersMscOML(HDC, INT64, INT64, INT64);
extern INT64 WINAPI wglSwapLayerBuffersMscOML(HDC, int, INT64, INT64, INT64);
extern BOOL WINAPI wglWaitForMscOML(HDC, INT64, INT64, INT64, INT64*, INT64*, INT64*);
extern BOOL WINAPI wglWaitForSbcOML(HDC, INT64, INT64*, INT64*, INT64*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETSYNCVALUESOMLPROC = BOOL(WINAPI*)(HDC hdc, INT64* ust, INT64* msc, INT64* sbc);
using PFNWGLGETMSCRATEOMLPROC = BOOL(WINAPI*)(HDC hdc, INT32* numerator, INT32* denominator);
using PFNWGLSWAPBUFFERSMSCOMLPROC = INT64(WINAPI*)(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
using PFNWGLSWAPLAYERBUFFERSMSCOMLPROC = INT64(WINAPI*)(HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
using PFNWGLWAITFORMSCOMLPROC = BOOL(WINAPI*)(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64* ust, INT64* msc, INT64* sbc);
using PFNWGLWAITFORSBCOMLPROC = BOOL(WINAPI*)(HDC hdc, INT64 target_sbc, INT64* ust, INT64* msc, INT64* sbc);
#endif

#ifndef WGL_I3D_digital_video_control
#define WGL_I3D_digital_video_control 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetDigitalVideoParametersI3D(HDC, int, int*);
extern BOOL WINAPI wglSetDigitalVideoParametersI3D(HDC, int, const int*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETDIGITALVIDEOPARAMETERSI3DPROC = BOOL(WINAPI*)(HDC hDC, int iAttribute, int* piValue);
using PFNWGLSETDIGITALVIDEOPARAMETERSI3DPROC = BOOL(WINAPI*)(HDC hDC, int iAttribute, const int* piValue);
#endif

#ifndef WGL_I3D_gamma
#define WGL_I3D_gamma 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetGammaTableParametersI3D(HDC, int, int*);
extern BOOL WINAPI wglSetGammaTableParametersI3D(HDC, int, const int*);
extern BOOL WINAPI wglGetGammaTableI3D(HDC, int, USHORT*, USHORT*, USHORT*);
extern BOOL WINAPI wglSetGammaTableI3D(HDC, int, const USHORT*, const USHORT*, const USHORT*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETGAMMATABLEPARAMETERSI3DPROC = BOOL(WINAPI*)(HDC hDC, int iAttribute, int* piValue);
using PFNWGLSETGAMMATABLEPARAMETERSI3DPROC = BOOL(WINAPI*)(HDC hDC, int iAttribute, const int* piValue);
using PFNWGLGETGAMMATABLEI3DPROC = BOOL(WINAPI*)(HDC hDC, int iEntries, USHORT* puRed, USHORT* puGreen, USHORT* puBlue);
using PFNWGLSETGAMMATABLEI3DPROC = BOOL(WINAPI*)(HDC hDC, int iEntries, const USHORT* puRed, const USHORT* puGreen, const USHORT* puBlue);
#endif

#ifndef WGL_I3D_genlock
#define WGL_I3D_genlock 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglEnableGenlockI3D(HDC);
extern BOOL WINAPI wglDisableGenlockI3D(HDC);
extern BOOL WINAPI wglIsEnabledGenlockI3D(HDC, BOOL*);
extern BOOL WINAPI wglGenlockSourceI3D(HDC, UINT);
extern BOOL WINAPI wglGetGenlockSourceI3D(HDC, UINT*);
extern BOOL WINAPI wglGenlockSourceEdgeI3D(HDC, UINT);
extern BOOL WINAPI wglGetGenlockSourceEdgeI3D(HDC, UINT*);
extern BOOL WINAPI wglGenlockSampleRateI3D(HDC, UINT);
extern BOOL WINAPI wglGetGenlockSampleRateI3D(HDC, UINT*);
extern BOOL WINAPI wglGenlockSourceDelayI3D(HDC, UINT);
extern BOOL WINAPI wglGetGenlockSourceDelayI3D(HDC, UINT*);
extern BOOL WINAPI wglQueryGenlockMaxSourceDelayI3D(HDC, UINT*, UINT*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLENABLEGENLOCKI3DPROC = BOOL(WINAPI*)(HDC hDC);
using PFNWGLDISABLEGENLOCKI3DPROC = BOOL(WINAPI*)(HDC hDC);
using PFNWGLISENABLEDGENLOCKI3DPROC = BOOL(WINAPI*)(HDC hDC, BOOL* pFlag);
using PFNWGLGENLOCKSOURCEI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT uSource);
using PFNWGLGETGENLOCKSOURCEI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT* uSource);
using PFNWGLGENLOCKSOURCEEDGEI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT uEdge);
using PFNWGLGETGENLOCKSOURCEEDGEI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT* uEdge);
using PFNWGLGENLOCKSAMPLERATEI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT uRate);
using PFNWGLGETGENLOCKSAMPLERATEI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT* uRate);
using PFNWGLGENLOCKSOURCEDELAYI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT uDelay);
using PFNWGLGETGENLOCKSOURCEDELAYI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT* uDelay);
using PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3DPROC = BOOL(WINAPI*)(HDC hDC, UINT* uMaxLineDelay, UINT* uMaxPixelDelay);
#endif

#ifndef WGL_I3D_image_buffer
#define WGL_I3D_image_buffer 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern LPVOID WINAPI wglCreateImageBufferI3D(HDC, DWORD, UINT);
extern BOOL WINAPI wglDestroyImageBufferI3D(HDC, LPVOID);
extern BOOL WINAPI wglAssociateImageBufferEventsI3D(HDC, const HANDLE*, const LPVOID*, const DWORD*, UINT);
extern BOOL WINAPI wglReleaseImageBufferEventsI3D(HDC, const LPVOID*, UINT);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLCREATEIMAGEBUFFERI3DPROC = LPVOID(WINAPI*)(HDC hDC, DWORD dwSize, UINT uFlags);
using PFNWGLDESTROYIMAGEBUFFERI3DPROC = BOOL(WINAPI*)(HDC hDC, LPVOID pAddress);
using PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3DPROC = BOOL(WINAPI*)(HDC hDC, const HANDLE* pEvent, const LPVOID* pAddress, const DWORD* pSize, UINT count);
using PFNWGLRELEASEIMAGEBUFFEREVENTSI3DPROC = BOOL(WINAPI*)(HDC hDC, const LPVOID* pAddress, UINT count);
#endif

#ifndef WGL_I3D_swap_frame_lock
#define WGL_I3D_swap_frame_lock 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglEnableFrameLockI3D(void);
extern BOOL WINAPI wglDisableFrameLockI3D(void);
extern BOOL WINAPI wglIsEnabledFrameLockI3D(BOOL*);
extern BOOL WINAPI wglQueryFrameLockMasterI3D(BOOL*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLENABLEFRAMELOCKI3DPROC = BOOL(WINAPI*)(void);
using PFNWGLDISABLEFRAMELOCKI3DPROC = BOOL(WINAPI*)(void);
using PFNWGLISENABLEDFRAMELOCKI3DPROC = BOOL(WINAPI*)(BOOL* pFlag);
using PFNWGLQUERYFRAMELOCKMASTERI3DPROC = BOOL(WINAPI*)(BOOL* pFlag);
#endif

#ifndef WGL_I3D_swap_frame_usage
#define WGL_I3D_swap_frame_usage 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetFrameUsageI3D(float*);
extern BOOL WINAPI wglBeginFrameTrackingI3D(void);
extern BOOL WINAPI wglEndFrameTrackingI3D(void);
extern BOOL WINAPI wglQueryFrameTrackingI3D(DWORD*, DWORD*, float*);
#endif /* WGL_WGLEXT_PROTOTYPES */
using PFNWGLGETFRAMEUSAGEI3DPROC = BOOL(WINAPI*)(float* pUsage);
using PFNWGLBEGINFRAMETRACKINGI3DPROC = BOOL(WINAPI*)(void);
using PFNWGLENDFRAMETRACKINGI3DPROC = BOOL(WINAPI*)(void);
using PFNWGLQUERYFRAMETRACKINGI3DPROC = BOOL(WINAPI*)(DWORD* pFrameCount, DWORD* pMissedFrames, float* pLastMissedUsage);
#endif


#ifdef __cplusplus
}
#endif

#endif
