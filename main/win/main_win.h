/***************************************************************************
						  main_win.h  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince
	email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Windows.h>
#include <string>
#include <functional>
#include <filesystem>
#define MUPEN_VERSION "Mupen 64 1.1.8-2"

#define WM_FOCUS_MAIN_WINDOW (WM_USER + 17)
#define WM_EXECUTE_DISPATCHER (WM_USER + 18)

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam,
                                LPARAM lParam);

extern DWORD g_ui_thread_id;

extern int last_wheel_delta;

extern HWND mainHWND;
extern HINSTANCE app_instance;

extern HWND hwnd_plug;
extern DWORD start_rom_id;

extern std::string app_path;

/**
 * \brief Whether the statusbar needs to be updated with new input information
 */
extern bool is_primary_statusbar_invalidated;

/**
 * \brief Pauses the emulation during the object's lifetime, resuming it if previously paused upon being destroyed
 */
struct BetterEmulationLock
{
private:
	bool was_paused;

public:
	BetterEmulationLock();
	~BetterEmulationLock();
};

typedef struct
{
	long width;
	long height;
	long statusbar_height;
} t_window_info;

extern t_window_info window_info;

t_window_info get_window_info();
