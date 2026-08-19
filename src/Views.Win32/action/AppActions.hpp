/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
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
const std::string APP = "Mupen64 > ";

const std::string LOAD_ROM_DIRECT = APP + "File > # Load ROM...";
const std::string LOAD_ROM = APP + "File > Load ROM...";
const std::string CLOSE_ROM = APP + "File > Close ROM";
const std::string RESET_ROM = APP + "File > Reset ROM";
const std::string REFRESH_ROM_LIST = APP + "File > Refresh ROM List ---";
const std::string RECENT_ROMS = APP + "File > Recent ROMs ---";
const std::string EXIT = APP + "File > Exit";

const std::string PAUSE = APP + "Emulation > Pause";
const std::string SPEED_DOWN = APP + "Emulation > Speed Down";
const std::string SPEED_UP = APP + "Emulation > Speed Up";
const std::string SPEED_RESET = APP + "Emulation > Reset Speed";
const std::string FAST_FORWARD = APP + "Emulation > Fast-Forward";
const std::string GS_BUTTON = APP + "Emulation > GS Button ---";
const std::string FRAME_ADVANCE = APP + "Emulation > Frame Advance";
const std::string MULTI_FRAME_ADVANCE_DIRECT = APP + "Emulation > # Multi-Frame Advance...";
const std::string MULTI_FRAME_ADVANCE = APP + "Emulation > Multi-Frame Advance";
const std::string MULTI_FRAME_ADVANCE_INCREMENT = APP + "Emulation > Multi-Frame Advance +1";
const std::string MULTI_FRAME_ADVANCE_DECREMENT = APP + "Emulation > Multi-Frame Advance -1";
const std::string MULTI_FRAME_ADVANCE_RESET = APP + "Emulation > Multi-Frame Advance Reset ---";
const std::string SAVE_CURRENT_SLOT = APP + "Emulation > Save State > Save Current Slot";
const std::string SAVE_STATE_FILE = APP + "Emulation > Save State > Save as File... ---";
const std::string LOAD_CURRENT_SLOT = APP + "Emulation > Load State > Load Current Slot";
const std::string LOAD_STATE_FILE = APP + "Emulation > Load State > Load from File... ---";
const std::string SAVE_SLOT_X = APP + "Emulation > Save State > Save Slot {}";
const std::string LOAD_SLOT_X = APP + "Emulation > Load State > Load Slot {}";
const std::string SELECT_SLOT = APP + "Emulation > Current State Slot > ";
const std::string SELECT_SLOT_X = SELECT_SLOT + "Slot {}";
const std::string UNDO_LOAD_STATE = APP + "Emulation > Undo Load State";

const std::string VIDEO_SETTINGS = APP + "Options > Plugin Settings --- > Video Settings";
const std::string AUDIO_SETTINGS = APP + "Options > Plugin Settings --- > Audio Settings";
const std::string INPUT_SETTINGS = APP + "Options > Plugin Settings --- > Input Settings";
const std::string RSP_SETTINGS = APP + "Options > Plugin Settings --- > RSP Settings";
const std::string STATUSBAR = APP + "Options > Statusbar ---";
const std::string SETTINGS = APP + "Options > Settings";

const std::string START_MOVIE_RECORDING_DIRECT = APP + "Movie > # Start Movie Recording...";
const std::string START_MOVIE_RECORDING = APP + "Movie > Start Movie Recording";
const std::string START_MOVIE_PLAYBACK_DIRECT = APP + "Movie > # Start Movie Playback...";
const std::string START_MOVIE_PLAYBACK = APP + "Movie > Start Movie Playback";
const std::string CONTINUE_MOVIE_RECORDING = APP + "Movie > Continue Movie Recording ---";
const std::string STOP_MOVIE = APP + "Movie > Stop Movie";
const std::string CREATE_MOVIE_BACKUP = APP + "Movie > Create Movie Backup ---";
const std::string RECENT_MOVIES = APP + "Movie > Recent Movies ---";
const std::string LOOP_MOVIE_PLAYBACK = APP + "Movie > Loop Movie Playback";
const std::string READONLY = APP + "Movie > Read-Only";
const std::string WAIT_AT_MOVIE_END = APP + "Movie > Wait at Movie End";

const std::string COMMAND_PALETTE = APP + "Utilities > Command Palette ---";
const std::string PIANO_ROLL = APP + "Utilities > Piano Roll";
const std::string CHEATS = APP + "Utilities > Cheats";
const std::string SEEK_TO_DIRECT = APP + "Utilities > # Seek...";
const std::string SEEK_TO = APP + "Utilities > Seek...";
const std::string USAGE_STATISTICS = APP + "Utilities > Usage Statistics ---";
const std::string CORE_INFORMATION = APP + "Utilities > Core Information";
const std::string START_TRACE_LOGGER = APP + "Utilities > Start Trace Logger...";
const std::string STOP_TRACE_LOGGER = APP + "Utilities > Stop Trace Logger ---";
const std::string VIDEO_CAPTURE = APP + "Utilities > Video Capture > ";
const std::string VIDEO_CAPTURE_START_DIRECT = VIDEO_CAPTURE + "# Start Capture...";
const std::string VIDEO_CAPTURE_START = VIDEO_CAPTURE + "Start Capture...";
const std::string VIDEO_CAPTURE_START_PRESET = VIDEO_CAPTURE + "Start Capture from Preset... ---";
const std::string VIDEO_CAPTURE_STOP = VIDEO_CAPTURE + "Stop Capture ---";
const std::string SCREENSHOT = VIDEO_CAPTURE + "Take Screenshot";

const std::string CHECK_FOR_UPDATES = APP + "Help > Check for Updates";
const std::string ABOUT = APP + "Help > About";

const std::string LOAD_SCRIPT_DIRECT = APP + "Lua Script > # Load Script...";
const std::string SHOW_INSTANCES = APP + "Lua Script > Show Instances ---";
const std::string RECENT_SCRIPTS = APP + "Lua Script > Recent Scripts ---";
const std::string STOP_ALL = APP + "Lua Script > Stop All";
const std::string CLOSE_ALL = APP + "Lua Script > Close All";

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
void load_rom_from_path(const std::filesystem::path &path);
} // namespace AppActions
