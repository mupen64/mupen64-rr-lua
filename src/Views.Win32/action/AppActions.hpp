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
const std::string APP = "mupen64";

const std::string LOAD_ROM_DIRECT = APP + ".file.load-rom-direct";
const std::string LOAD_ROM = APP + ".file.load-rom";
const std::string CLOSE_ROM = APP + ".file.close-rom";
const std::string RESET_ROM = APP + ".file.reset-rom";
const std::string REFRESH_ROM_LIST = APP + ".file.refresh-rom-list";
const std::string RECENT_ROMS = APP + ".file.recent-roms";
const std::string EXIT = APP + ".file.exit";

const std::string PAUSE = APP + ".emulation.pause";
const std::string SPEED_DOWN = APP + ".emulation.speed-down";
const std::string SPEED_UP = APP + ".emulation.speed-up";
const std::string SPEED_RESET = APP + ".emulation.reset-speed";
const std::string FAST_FORWARD = APP + ".emulation.fast-forward";
const std::string GS_BUTTON = APP + ".emulation.gs-button";
const std::string FRAME_ADVANCE = APP + ".emulation.frame-advance";
const std::string MULTI_FRAME_ADVANCE_DIRECT = APP + ".emulation.multi-frame-advance-direct";
const std::string MULTI_FRAME_ADVANCE = APP + ".emulation.multi-frame-advance";
const std::string MULTI_FRAME_ADVANCE_INCREMENT = APP + ".emulation.multi-frame-advance-increment";
const std::string MULTI_FRAME_ADVANCE_DECREMENT = APP + ".emulation.multi-frame-advance-decrement";
const std::string MULTI_FRAME_ADVANCE_RESET = APP + ".emulation.multi-frame-advance-reset";
const std::string SAVE_CURRENT_SLOT = APP + ".emulation.save-state.save-current-slot";
const std::string SAVE_STATE_FILE = APP + ".emulation.save-state.save-as-file";
const std::string LOAD_CURRENT_SLOT = APP + ".emulation.load-state.load-current-slot";
const std::string LOAD_STATE_FILE = APP + ".emulation.load-state.load-from-file";
const std::string SAVE_SLOT_X = APP + ".emulation.save-state.save-slot-{}";
const std::string LOAD_SLOT_X = APP + ".emulation.load-state.load-slot-{}";
const std::string SELECT_SLOT = APP + ".emulation.current-state-slot";
const std::string SELECT_SLOT_X = SELECT_SLOT + ".slot-{}";
const std::string UNDO_LOAD_STATE = APP + ".emulation.undo-load-state";

const std::string VIDEO_SETTINGS = APP + ".options.plugin-settings.video-settings";
const std::string AUDIO_SETTINGS = APP + ".options.plugin-settings.audio-settings";
const std::string INPUT_SETTINGS = APP + ".options.plugin-settings.input-settings";
const std::string RSP_SETTINGS = APP + ".options.plugin-settings.rsp-settings";
const std::string STATUSBAR = APP + ".options.statusbar";
const std::string SETTINGS = APP + ".options.settings";

const std::string START_MOVIE_RECORDING_DIRECT = APP + ".movie.start-movie-recording-direct";
const std::string START_MOVIE_RECORDING = APP + ".movie.start-movie-recording";
const std::string START_MOVIE_PLAYBACK_DIRECT = APP + ".movie.start-movie-playback-direct";
const std::string START_MOVIE_PLAYBACK = APP + ".movie.start-movie-playback";
const std::string CONTINUE_MOVIE_RECORDING = APP + ".movie.continue-movie-recording";
const std::string STOP_MOVIE = APP + ".movie.stop-movie";
const std::string CREATE_MOVIE_BACKUP = APP + ".movie.create-movie-backup";
const std::string RECENT_MOVIES = APP + ".movie.recent-movies";
const std::string LOOP_MOVIE_PLAYBACK = APP + ".movie.loop-movie-playback";
const std::string READONLY = APP + ".movie.read-only";
const std::string WAIT_AT_MOVIE_END = APP + ".movie.wait-at-movie-end";

const std::string COMMAND_PALETTE = APP + ".utilities.command-palette";
const std::string PIANO_ROLL = APP + ".utilities.piano-roll";
const std::string CHEATS = APP + ".utilities.cheats";
const std::string SEEK_TO_DIRECT = APP + ".utilities.seek-direct";
const std::string SEEK_TO = APP + ".utilities.seek";
const std::string USAGE_STATISTICS = APP + ".utilities.usage-statistics";
const std::string CORE_INFORMATION = APP + ".utilities.core-information";
const std::string START_TRACE_LOGGER = APP + ".utilities.start-trace-logger";
const std::string STOP_TRACE_LOGGER = APP + ".utilities.stop-trace-logger";
const std::string VIDEO_CAPTURE = APP + ".utilities.video-capture";
const std::string VIDEO_CAPTURE_START_DIRECT = VIDEO_CAPTURE + ".start-capture-direct";
const std::string VIDEO_CAPTURE_START = VIDEO_CAPTURE + ".start-capture";
const std::string VIDEO_CAPTURE_START_PRESET = VIDEO_CAPTURE + ".start-capture-from-preset";
const std::string VIDEO_CAPTURE_STOP = VIDEO_CAPTURE + ".stop-capture";
const std::string SCREENSHOT = VIDEO_CAPTURE + ".take-screenshot";

const std::string CHECK_FOR_UPDATES = APP + ".help.check-for-updates";
const std::string ABOUT = APP + ".help.about";

const std::string LOAD_SCRIPT_DIRECT = APP + ".lua-script.load-script-direct";
const std::string SHOW_INSTANCES = APP + ".lua-script.show-instances";
const std::string RECENT_SCRIPTS = APP + ".lua-script.recent-scripts";
const std::string STOP_ALL = APP + ".lua-script.stop-all";
const std::string CLOSE_ALL = APP + ".lua-script.close-all";

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
