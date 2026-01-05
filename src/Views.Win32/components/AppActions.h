/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

// Throwaway actions which can be spammed get keys as to not clog up the async executor queue
#define ASYNC_KEY_CLOSE_ROM (1)
#define ASYNC_KEY_START_ROM (2)
#define ASYNC_KEY_RESET_ROM (3)
#define ASYNC_KEY_PLAY_MOVIE (4)

/**
 * \brief A module responsible for implementing standard application actions.
 */
namespace AppActions
{
const std::wstring APP = L"Mupen64 > ";

const std::wstring LOAD_ROM = APP + L"File > Load ROM...";
const std::wstring CLOSE_ROM = APP + L"File > Close ROM";
const std::wstring RESET_ROM = APP + L"File > Reset ROM";
const std::wstring REFRESH_ROM_LIST = APP + L"File > Refresh ROM List ---";
const std::wstring RECENT_ROMS = APP + L"File > Recent ROMs ---";
const std::wstring EXIT = APP + L"File > Exit";

const std::wstring PAUSE = APP + L"Emulation > Pause";
const std::wstring SPEED_DOWN = APP + L"Emulation > Speed Down";
const std::wstring SPEED_UP = APP + L"Emulation > Speed Up";
const std::wstring SPEED_RESET = APP + L"Emulation > Reset Speed";
const std::wstring FAST_FORWARD = APP + L"Emulation > Fast-Forward";
const std::wstring GS_BUTTON = APP + L"Emulation > GS Button ---";
const std::wstring FRAME_ADVANCE = APP + L"Emulation > Frame Advance";
const std::wstring MULTI_FRAME_ADVANCE = APP + L"Emulation > Multi-Frame Advance";
const std::wstring MULTI_FRAME_ADVANCE_INCREMENT = APP + L"Emulation > Multi-Frame Advance +1";
const std::wstring MULTI_FRAME_ADVANCE_DECREMENT = APP + L"Emulation > Multi-Frame Advance -1";
const std::wstring MULTI_FRAME_ADVANCE_RESET = APP + L"Emulation > Multi-Frame Advance Reset ---";
const std::wstring SAVE_CURRENT_SLOT = APP + L"Emulation > Save State > Save Current Slot";
const std::wstring SAVE_STATE_FILE = APP + L"Emulation > Save State > Save as File... ---";
const std::wstring LOAD_CURRENT_SLOT = APP + L"Emulation > Load State > Load Current Slot";
const std::wstring LOAD_STATE_FILE = APP + L"Emulation > Load State > Load from File... ---";
const std::wstring SAVE_SLOT_X = APP + L"Emulation > Save State > Save Slot {}";
const std::wstring LOAD_SLOT_X = APP + L"Emulation > Load State > Load Slot {}";
const std::wstring SELECT_SLOT = APP + L"Emulation > Current State Slot > ";
const std::wstring SELECT_SLOT_X = SELECT_SLOT + L"Slot {}";
const std::wstring UNDO_LOAD_STATE = APP + L"Emulation > Undo Load State";

const std::wstring FULL_SCREEN = APP + L"Options > Full Screen ---";
const std::wstring VIDEO_SETTINGS = APP + L"Options > Plugin Settings --- > Video Settings";
const std::wstring AUDIO_SETTINGS = APP + L"Options > Plugin Settings --- > Audio Settings";
const std::wstring INPUT_SETTINGS = APP + L"Options > Plugin Settings --- > Input Settings";
const std::wstring RSP_SETTINGS = APP + L"Options > Plugin Settings --- > RSP Settings";
const std::wstring STATUSBAR = APP + L"Options > Statusbar ---";
const std::wstring SETTINGS = APP + L"Options > Settings";

const std::wstring START_MOVIE_RECORDING = APP + L"Movie > Start Movie Recording";
const std::wstring START_MOVIE_PLAYBACK = APP + L"Movie > Start Movie Playback";
const std::wstring CONTINUE_MOVIE_RECORDING = APP + L"Movie > Continue Movie Recording ---";
const std::wstring STOP_MOVIE = APP + L"Movie > Stop Movie";
const std::wstring CREATE_MOVIE_BACKUP = APP + L"Movie > Create Movie Backup ---";
const std::wstring RECENT_MOVIES = APP + L"Movie > Recent Movies ---";
const std::wstring LOOP_MOVIE_PLAYBACK = APP + L"Movie > Loop Movie Playback";
const std::wstring READONLY = APP + L"Movie > Read-Only";
const std::wstring WAIT_AT_MOVIE_END = APP + L"Movie > Wait at Movie End";

const std::wstring COMMAND_PALETTE = APP + L"Utilities > Command Palette ---";
const std::wstring PIANO_ROLL = APP + L"Utilities > Piano Roll";
const std::wstring CHEATS = APP + L"Utilities > Cheats";
const std::wstring SEEK_TO = APP + L"Utilities > Seek...";
const std::wstring USAGE_STATISTICS = APP + L"Utilities > Usage Statistics ---";
const std::wstring CORE_INFORMATION = APP + L"Utilities > Core Information";
const std::wstring DEBUGGER = APP + L"Utilities > Debugger";
const std::wstring START_TRACE_LOGGER = APP + L"Utilities > Start Trace Logger...";
const std::wstring STOP_TRACE_LOGGER = APP + L"Utilities > Stop Trace Logger ---";
const std::wstring VIDEO_CAPTURE = APP + L"Utilities > Video Capture > ";
const std::wstring VIDEO_CAPTURE_START = VIDEO_CAPTURE + L"Start Capture...";
const std::wstring VIDEO_CAPTURE_START_PRESET = VIDEO_CAPTURE + L"Start Capture from Preset... ---";
const std::wstring VIDEO_CAPTURE_STOP = VIDEO_CAPTURE + L"Stop Capture ---";
const std::wstring SCREENSHOT = VIDEO_CAPTURE + L"Take Screenshot";

const std::wstring CHECK_FOR_UPDATES = APP + L"Help > Check for Updates";
const std::wstring ABOUT = APP + L"Help > About";

const std::wstring SHOW_INSTANCES = APP + L"Lua Script > Show Instances ---";
const std::wstring RECENT_SCRIPTS = APP + L"Lua Script > Recent Scripts ---";
const std::wstring CLOSE_ALL = APP + L"Lua Script > Close All";

/**
 * \brief Initializes the module.
 */
void init();

/**
 * \brief Adds the standard app actions to the action registry.
 */
void add();

void update_core_fast_forward();

/**
 * \brief Starts loading a ROM from the given path.
 * \param path A path.
 */
void load_rom_from_path(const std::wstring &path);
} // namespace AppActions
