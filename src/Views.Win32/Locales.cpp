/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/I18n.hpp>
#include <action/AppActions.hpp>
#include <Locales.hpp>

namespace Locales
{
void register_app_actions()
{
    const auto add_name = [](std::string_view id, std::string_view name) {
        I18n::get().add(std::string(id), std::string(name), "en");
    };

    add_name(AppActions::APP, "Mupen64");
    add_name(AppActions::APP + ".file", "File");
    add_name(AppActions::LOAD_ROM_DIRECT, "Load ROM...");
    add_name(AppActions::LOAD_ROM, "Load ROM...");
    add_name(AppActions::CLOSE_ROM, "Close ROM");
    add_name(AppActions::RESET_ROM, "Reset ROM");
    add_name(AppActions::REFRESH_ROM_LIST, "Refresh ROM List");
    add_name(AppActions::RECENT_ROMS, "Recent ROMs");
    add_name(AppActions::EXIT, "Exit");

    add_name(AppActions::APP + ".emulation", "Emulation");
    add_name(AppActions::PAUSE, "Pause");
    add_name(AppActions::SPEED_DOWN, "Speed Down");
    add_name(AppActions::SPEED_UP, "Speed Up");
    add_name(AppActions::SPEED_RESET, "Reset Speed");
    add_name(AppActions::FAST_FORWARD, "Fast-Forward");
    add_name(AppActions::GS_BUTTON, "GS Button");
    add_name(AppActions::FRAME_ADVANCE, "Frame Advance");
    add_name(AppActions::MULTI_FRAME_ADVANCE_DIRECT, "Multi-Frame Advance...");
    add_name(AppActions::MULTI_FRAME_ADVANCE, "Multi-Frame Advance");
    add_name(AppActions::MULTI_FRAME_ADVANCE_INCREMENT, "Multi-Frame Advance +1");
    add_name(AppActions::MULTI_FRAME_ADVANCE_DECREMENT, "Multi-Frame Advance -1");
    add_name(AppActions::MULTI_FRAME_ADVANCE_RESET, "Multi-Frame Advance Reset");
    add_name(AppActions::APP + ".emulation.save-state", "Save State");
    add_name(AppActions::SAVE_CURRENT_SLOT, "Save Current Slot");
    add_name(AppActions::SAVE_STATE_FILE, "Save as File...");
    add_name(AppActions::APP + ".emulation.load-state", "Load State");
    add_name(AppActions::LOAD_CURRENT_SLOT, "Load Current Slot");
    add_name(AppActions::LOAD_STATE_FILE, "Load from File...");
    add_name(AppActions::UNDO_LOAD_STATE, "Undo Load State");
    add_name(AppActions::SELECT_SLOT, "Current State Slot");

    add_name(AppActions::APP + ".options", "Options");
    add_name(AppActions::APP + ".options.plugin-settings", "Plugin Settings");
    add_name(AppActions::VIDEO_SETTINGS, "Video Settings");
    add_name(AppActions::AUDIO_SETTINGS, "Audio Settings");
    add_name(AppActions::INPUT_SETTINGS, "Input Settings");
    add_name(AppActions::RSP_SETTINGS, "RSP Settings");
    add_name(AppActions::STATUSBAR, "Statusbar");
    add_name(AppActions::SETTINGS, "Settings");

    add_name(AppActions::APP + ".movie", "Movie");
    add_name(AppActions::START_MOVIE_RECORDING_DIRECT, "Start Movie Recording...");
    add_name(AppActions::START_MOVIE_RECORDING, "Start Movie Recording");
    add_name(AppActions::START_MOVIE_PLAYBACK_DIRECT, "Start Movie Playback...");
    add_name(AppActions::START_MOVIE_PLAYBACK, "Start Movie Playback");
    add_name(AppActions::CONTINUE_MOVIE_RECORDING, "Continue Movie Recording");
    add_name(AppActions::STOP_MOVIE, "Stop Movie");
    add_name(AppActions::CREATE_MOVIE_BACKUP, "Create Movie Backup");
    add_name(AppActions::RECENT_MOVIES, "Recent Movies");
    add_name(AppActions::LOOP_MOVIE_PLAYBACK, "Loop Movie Playback");
    add_name(AppActions::READONLY, "Read-Only");
    add_name(AppActions::WAIT_AT_MOVIE_END, "Wait at Movie End");

    add_name(AppActions::APP + ".utilities", "Utilities");
    add_name(AppActions::COMMAND_PALETTE, "Command Palette");
    add_name(AppActions::PIANO_ROLL, "Piano Roll");
    add_name(AppActions::CHEATS, "Cheats");
    add_name(AppActions::SEEK_TO_DIRECT, "Seek...");
    add_name(AppActions::SEEK_TO, "Seek...");
    add_name(AppActions::USAGE_STATISTICS, "Usage Statistics");
    add_name(AppActions::CORE_INFORMATION, "Core Information");
    add_name(AppActions::START_TRACE_LOGGER, "Start Trace Logger...");
    add_name(AppActions::STOP_TRACE_LOGGER, "Stop Trace Logger");
    add_name(AppActions::VIDEO_CAPTURE, "Video Capture");
    add_name(AppActions::VIDEO_CAPTURE_START_DIRECT, "Start Capture...");
    add_name(AppActions::VIDEO_CAPTURE_START, "Start Capture...");
    add_name(AppActions::VIDEO_CAPTURE_START_PRESET, "Start Capture from Preset...");
    add_name(AppActions::VIDEO_CAPTURE_STOP, "Stop Capture");
    add_name(AppActions::SCREENSHOT, "Take Screenshot");

    add_name(AppActions::APP + ".help", "Help");
    add_name(AppActions::CHECK_FOR_UPDATES, "Check for Updates");
    add_name(AppActions::ABOUT, "About");

    add_name(AppActions::APP + ".lua-script", "Lua Script");
    add_name(AppActions::LOAD_SCRIPT_DIRECT, "Load Script...");
    add_name(AppActions::SHOW_INSTANCES, "Show Instances");
    add_name(AppActions::RECENT_SCRIPTS, "Recent Scripts");
    add_name(AppActions::STOP_ALL, "Stop All");
    add_name(AppActions::CLOSE_ALL, "Close All");

    I18n::get().set_locale("en");
}
} // namespace Locales
