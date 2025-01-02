#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

/**
 * \brief Action that can be triggered by a hotkey
 */
enum class Action {
  None,
  FastforwardOn,
  FastforwardOff,
  GamesharkOn,
  GamesharkOff,
  SpeedDown,
  SpeedUp,
  FrameAdvance,
  Pause,
  ToggleReadOnly,
  ToggleMovieLoop,
  StartMoviePlayback,
  StartMovieRecording,
  StopMovie,
  TakeScreenshot,
  PlayLatestMovie,
  LoadLatestScript,
  NewLua,
  CloseAllLua,
  LoadRom,
  CloseRom,
  ResetRom,
  LoadLatestRom,
  Fullscreen,
  Settings,
  ToggleStatusbar,
  RefreshRomBrowser,
  OpenSeeker,
  OpenRunner,
  OpenPianoRoll,
  OpenCheats,
  SaveSlot,
  LoadSlot,
  SaveAs,
  LoadAs,
  UndoLoadState,
  SaveSlot1,
  SaveSlot2,
  SaveSlot3,
  SaveSlot4,
  SaveSlot5,
  SaveSlot6,
  SaveSlot7,
  SaveSlot8,
  SaveSlot9,
  SaveSlot10,
  LoadSlot1,
  LoadSlot2,
  LoadSlot3,
  LoadSlot4,
  LoadSlot5,
  LoadSlot6,
  LoadSlot7,
  LoadSlot8,
  LoadSlot9,
  LoadSlot10,
  SelectSlot1,
  SelectSlot2,
  SelectSlot3,
  SelectSlot4,
  SelectSlot5,
  SelectSlot6,
  SelectSlot7,
  SelectSlot8,
  SelectSlot9,
  SelectSlot10,
};

enum class PresenterType : int32_t { DirectComposition, GDI };

/**
 * \brief A data stream encoder type
 */
enum class EncoderType : int32_t {
  /**
   * \brief Microsoft's VFW
   */
  VFW,

  /**
   * \brief The FFmpeg library
   */
  FFmpeg,
};

/**
 * \brief The statusbar layout preset.
 */
enum class StatusbarLayout : int32_t {
  /**
   * The legacy layout.
   */
  Classic,

  /**
   * The new layout containing additional information.
   */
  Modern,

  /**
   * The new layout, but with a section for read-only status.
   */
  ModernWithReadonly,
};

typedef struct Hotkey {
  std::wstring identifier;
  int32_t key;
  int32_t ctrl;
  int32_t shift;
  int32_t alt;
  Action down_cmd;
  Action up_cmd;
} t_hotkey;

#pragma pack(push, 1)
typedef struct Config {
#pragma region Hotkeys
  t_hotkey fast_forward_hotkey;
  t_hotkey gs_hotkey;
  t_hotkey speed_down_hotkey;
  t_hotkey speed_up_hotkey;
  t_hotkey frame_advance_hotkey;
  t_hotkey pause_hotkey;
  t_hotkey toggle_read_only_hotkey;
  t_hotkey toggle_movie_loop_hotkey;
  t_hotkey start_movie_playback_hotkey;
  t_hotkey start_movie_recording_hotkey;
  t_hotkey stop_movie_hotkey;
  t_hotkey take_screenshot_hotkey;
  t_hotkey play_latest_movie_hotkey;
  t_hotkey load_latest_script_hotkey;
  t_hotkey new_lua_hotkey;
  t_hotkey close_all_lua_hotkey;
  t_hotkey load_rom_hotkey;
  t_hotkey close_rom_hotkey;
  t_hotkey reset_rom_hotkey;
  t_hotkey load_latest_rom_hotkey;
  t_hotkey fullscreen_hotkey;
  t_hotkey settings_hotkey;
  t_hotkey toggle_statusbar_hotkey;
  t_hotkey refresh_rombrowser_hotkey;
  t_hotkey seek_to_frame_hotkey;
  t_hotkey run_hotkey;
  t_hotkey piano_roll_hotkey;
  t_hotkey cheats_hotkey;
  t_hotkey save_current_hotkey;
  t_hotkey load_current_hotkey;
  t_hotkey save_as_hotkey;
  t_hotkey load_as_hotkey;
  t_hotkey undo_load_state_hotkey;
  t_hotkey save_to_slot_1_hotkey;
  t_hotkey save_to_slot_2_hotkey;
  t_hotkey save_to_slot_3_hotkey;
  t_hotkey save_to_slot_4_hotkey;
  t_hotkey save_to_slot_5_hotkey;
  t_hotkey save_to_slot_6_hotkey;
  t_hotkey save_to_slot_7_hotkey;
  t_hotkey save_to_slot_8_hotkey;
  t_hotkey save_to_slot_9_hotkey;
  t_hotkey save_to_slot_10_hotkey;
  t_hotkey load_from_slot_1_hotkey;
  t_hotkey load_from_slot_2_hotkey;
  t_hotkey load_from_slot_3_hotkey;
  t_hotkey load_from_slot_4_hotkey;
  t_hotkey load_from_slot_5_hotkey;
  t_hotkey load_from_slot_6_hotkey;
  t_hotkey load_from_slot_7_hotkey;
  t_hotkey load_from_slot_8_hotkey;
  t_hotkey load_from_slot_9_hotkey;
  t_hotkey load_from_slot_10_hotkey;
  t_hotkey select_slot_1_hotkey;
  t_hotkey select_slot_2_hotkey;
  t_hotkey select_slot_3_hotkey;
  t_hotkey select_slot_4_hotkey;
  t_hotkey select_slot_5_hotkey;
  t_hotkey select_slot_6_hotkey;
  t_hotkey select_slot_7_hotkey;
  t_hotkey select_slot_8_hotkey;
  t_hotkey select_slot_9_hotkey;
  t_hotkey select_slot_10_hotkey;

#pragma endregion

  /// <summary>
  /// The config protocol version
  /// </summary>
  int32_t version = 5;

  /// <summary>
  /// Statistic - Amount of state loads during recording
  /// </summary>
  uint64_t total_rerecords;

  /// <summary>
  /// Statistic - Amount of emulated frames
  /// </summary>
  uint64_t total_frames;

  /// <summary>
  /// The currently selected core type
  /// <para/>
  /// 0 - Cached Interpreter
  /// 1 - Dynamic Recompiler
  /// 2 - Pure Interpreter
  /// </summary>
  int32_t core_type = 1;

  /// <summary>
  /// The target FPS modifier as a percentage
  /// <para/>
  /// (100 = default)
  /// </summary>
  int32_t fps_modifier = 100;

  /// <summary>
  /// The frequency at which frames are skipped during fast-forward
  /// <para/>
  /// 0 = skip all frames
  /// 1 = skip no frames
  /// >0 = every nth frame is skipped
  /// </summary>
  int32_t frame_skip_frequency = 1;

  /// <summary>
  /// The current savestate slot index (0-9).
  /// </summary>
  int32_t st_slot;

  /// <summary>
  /// Whether fast-forward will mute audio
  /// This option improves performance by skipping additional doRspCycles calls,
  /// but may cause issues
  /// </summary>
  int32_t fastforward_silent;

  /// <summary>
  /// Whether lag frames will cause updateScreen to be invoked
  /// </summary>
  int32_t skip_rendering_lag;

  /// <summary>
  /// Maximum number of entries into the rom cache
  /// <para/>
  /// 0 = disabled
  /// </summary>
  int32_t rom_cache_size;

  /// <summary>
  /// Saves video buffer to savestates, slow!
  /// </summary>
  int32_t st_screenshot;

  /// <summary>
  /// Whether a playing movie will loop upon ending
  /// </summary>
  int32_t is_movie_loop_enabled;

  /// <summary>
  /// The CPU's counter factor. Higher values will generate less lag frames
  /// in-game at the cost of higher native CPU usage.
  /// </summary>
  int32_t counter_factor = 1;

  /// <summary>
  /// Whether emulation will pause when the main window loses focus
  /// </summary>
  int32_t is_unfocused_pause_enabled;

  /// <summary>
  /// Whether the statusbar is enabled
  /// </summary>
  int32_t is_statusbar_enabled = 1;

  /// <summary>
  /// Whether the statusbar is allowed to scale its segments down.
  /// </summary>
  int32_t statusbar_scale_down = 1;

  /// <summary>
  /// Whether the statusbar is allowed to scale its segments up.
  /// </summary>
  int32_t statusbar_scale_up;

  /// <summary>
  /// The statusbar layout.
  /// </summary>
  int32_t statusbar_layout = static_cast<int32_t>(StatusbarLayout::Modern);

  /// <summary>
  /// Whether plugins discovery is performed asynchronously. Removes potential
  /// waiting times in the config dialog.
  /// </summary>
  int32_t plugin_discovery_async = 1;

  /// <summary>
  /// Whether the default plugins directory will be used (otherwise, falls back
  /// to <see cref="plugins_directory"/>)
  /// </summary>
  int32_t is_default_plugins_directory_used = 1;

  /// <summary>
  /// Whether the default save directory will be used (otherwise, falls back to
  /// <see cref="saves_directory"/>)
  /// </summary>
  int32_t is_default_saves_directory_used = 1;

  /// <summary>
  /// Whether the default screenshot directory will be used (otherwise, falls
  /// back to <see cref="screenshots_directory"/>)
  /// </summary>
  int32_t is_default_screenshots_directory_used = 1;

  /// <summary>
  /// Whether the default backups directory will be used (otherwise, falls back
  /// to <see cref="backups_directory"/>)
  /// </summary>
  int32_t is_default_backups_directory_used = 1;

  /// <summary>
  /// The plugin directory
  /// </summary>
  std::wstring plugins_directory;

  /// <summary>
  /// The save directory
  /// </summary>
  std::wstring saves_directory;

  /// <summary>
  /// The screenshot directory
  /// </summary>
  std::wstring screenshots_directory;

  /// <summary>
  /// The savestate directory
  /// </summary>
  std::wstring states_path;

  /// <summary>
  /// The movie backups directory
  /// </summary>
  std::wstring backups_directory;

  /// <summary>
  /// The recently opened roms' paths
  /// </summary>
  std::vector<std::wstring> recent_rom_paths;

  /// <summary>
  /// Whether recently opened rom path collection is paused
  /// </summary>
  int32_t is_recent_rom_paths_frozen;

  /// <summary>
  /// The recently opened movies' paths
  /// </summary>
  std::vector<std::wstring> recent_movie_paths;

  /// <summary>
  /// Whether recently opened movie path collection is paused
  /// </summary>
  int32_t is_recent_movie_paths_frozen;

  /// <summary>
  /// Whether the rom browser will recursively search for roms beginning in the
  /// specified directories
  /// </summary>
  int32_t is_rombrowser_recursion_enabled;

  /// <summary>
  /// The paths to directories which are searched for roms
  /// </summary>
  std::vector<std::wstring> rombrowser_rom_paths;

  /// <summary>
  /// Whether rom resets are not recorded in movies
  /// </summary>
  int32_t is_reset_recording_enabled;

  /// <summary>
  /// The strategy to use when capturing video
  /// <para/>
  /// 0 = Use the video plugin's readScreen or read_video (MGE)
  /// 1 = Internal capture of window
  /// 2 = Internal capture of screen cropped to window
  /// 3 = Use the video plugin's readScreen or read_video (MGE) composited with
  /// lua scripts
  /// </summary>
  int32_t capture_mode = 3;

  /// <summary>
  /// The presenter to use for Lua scripts
  /// </summary>
  int32_t presenter_type =
      static_cast<int32_t>(PresenterType::DirectComposition);

  /// <summary>
  /// The encoder to use for capturing.
  /// </summary>
  int32_t encoder_type = static_cast<int32_t>(EncoderType::VFW);

  /// <summary>
  /// The delay (in milliseconds) before capturing the window
  /// <para/>
  /// May be useful when capturing other windows alongside mupen
  /// </summary>
  int32_t capture_delay;

  /// <summary>
  /// FFmpeg post-stream option format string which is used when capturing using
  /// the FFmpeg encoder type
  /// </summary>
  std::wstring ffmpeg_final_options =
      L"-y -f rawvideo -pixel_format bgr24 -video_size %dx%d -framerate %d -i "
      L"%s "
      L"-f s16le -sample_rate %d -ac 2 -channel_layout stereo -i %s "
      L"-c:v libx264 -preset fast -crf 23 -c:a copy -b:a 128k -vf \"vflip\" -f "
      L"mp4 %s";

  /// <summary>
  /// FFmpeg binary path
  /// </summary>
  std::wstring ffmpeg_path = L"C:\\ffmpeg\\bin\\ffmpeg.exe";

  /// <summary>
  /// The audio-video synchronization mode
  /// <para/>
  /// 0 - No Sync
  /// 1 - Audio Sync
  /// 2 - Video Sync
  /// </summary>
  int32_t synchronization_mode = 1;

  /// <summary>
  /// When enabled, mupen won't change the working directory to its current path
  /// at startup
  /// </summary>
  int32_t keep_default_working_directory;

  /// <summary>
  /// Whether the async executor is used for async calls. If disabled, a new
  /// thread is spawned for each call (legacy behaviour).
  /// </summary>
  int32_t use_async_executor;

  /// <summary>
  /// Whether the async executor will apply concurrency fuzzing. When enabled,
  /// task execution will be delayed to expose delayed task execution handling
  /// deficiencies at the callsite.
  /// </summary>
  int32_t async_executor_cuzz;

  /// <summary>
  /// The lua script path
  /// </summary>
  std::wstring lua_script_path;

  /// <summary>
  /// The recently opened lua scripts' paths
  /// </summary>
  std::vector<std::wstring> recent_lua_script_paths;

  /// <summary>
  /// Whether recently opened lua script path collection is paused
  /// </summary>
  int32_t is_recent_scripts_frozen;

  /// <summary>
  /// The save interval for warp modify savestates in frames
  /// </summary>
  int32_t seek_savestate_interval = 0;

  /// <summary>
  /// The maximum amount of warp modify savestates to keep in memory
  /// </summary>
  int32_t seek_savestate_max_count = 20;

  /// <summary>
  /// Whether piano roll edits are constrained to the column they started on
  /// </summary>
  int32_t piano_roll_constrain_edit_to_column;

  /// <summary>
  /// Maximum size of the undo/redo stack.
  /// </summary>
  int32_t piano_roll_undo_stack_size = 100;

  /// <summary>
  /// Whether the piano roll will try to keep the selection visible when the
  /// frame changes
  /// </summary>
  int32_t piano_roll_keep_selection_visible;

  /// <summary>
  /// Whether the piano roll will try to keep the playhead visible when the
  /// frame changes
  /// </summary>
  int32_t piano_roll_keep_playhead_visible;

  /// <summary>
  /// Whether undo savestate load functionality is enabled.
  /// </summary>
  int32_t st_undo_load = 1;

  /// <summary>
  /// SD card emulation
  /// </summary>
  int32_t use_summercart;

  /// <summary>
  /// Whether WiiVC emulation is enabled. Causes truncation instead of rounding
  /// when performing certain casts.
  /// </summary>
  int32_t wii_vc_emulation;

  /// <summary>
  /// Whether floating point exceptions will propagate and crash the emulator
  /// </summary>
  int32_t is_float_exception_propagation_enabled;

  /// <summary>
  /// Whether audio interrupts will be delayed
  /// </summary>
  int32_t is_audio_delay_enabled = 1;

  /// <summary>
  /// Whether jmp instructions will cause the following code to be JIT'd by
  /// dynarec
  /// </summary>
  int32_t is_compiled_jump_enabled = 1;

  /// <summary>
  /// The path of the currently selected video plugin
  /// </summary>
  std::wstring selected_video_plugin;

  /// <summary>
  /// The path of the currently selected audio plugin
  /// </summary>
  std::wstring selected_audio_plugin;

  /// <summary>
  /// The path of the currently selected input plugin
  /// </summary>
  std::wstring selected_input_plugin;

  /// <summary>
  /// The path of the currently selected RSP plugin
  /// </summary>
  std::wstring selected_rsp_plugin;

  /// <summary>
  /// The last known value of the record movie dialog's "start type" field
  /// </summary>
  int32_t last_movie_type = 1; // (MOVIE_START_FROM_SNAPSHOT)

  /// <summary>
  /// The last known value of the record movie dialog's "author" field
  /// </summary>
  std::wstring last_movie_author;

  /// <summary>
  /// The main window's X position
  /// </summary>
  int32_t window_x = ((int)0x80000000);

  /// <summary>
  /// The main window's Y position
  /// </summary>
  int32_t window_y = ((int)0x80000000);

  /// <summary>
  /// The main window's width
  /// </summary>
  int32_t window_width = 640;

  /// <summary>
  /// The main window's height
  /// </summary>
  int32_t window_height = 480;

  /// <summary>
  /// The width of rombrowser columns by index
  /// </summary>
  std::vector<std::int32_t> rombrowser_column_widths = {24, 240, 240, 120};

  /// <summary>
  /// The index of the currently sorted column, or -1 if none is sorted
  /// </summary>
  int32_t rombrowser_sorted_column;

  /// <summary>
  /// Whether the selected column is sorted in an ascending order
  /// </summary>
  int32_t rombrowser_sort_ascending = 1;

  /// <summary>
  /// A map of persistent path dialog IDs and the respective value
  /// </summary>
  std::map<std::wstring, std::wstring> persistent_folder_paths;

  /// <summary>
  /// The last selected settings tab's index.
  /// </summary>
  int32_t settings_tab;

  /// <summary>
  /// Resets the emulator faster, but may cause issues
  /// </summary>
  int32_t fast_reset;

  /// <summary>
  /// Whether VCR displays frame information relative to frame 0, not 1
  /// </summary>
  int32_t vcr_0_index;

  /// <summary>
  /// Increments the current slot when saving savestate to slot
  /// </summary>
  int32_t increment_slot;

  /// <summary>
  /// The movie frame to automatically pause at
  /// -1 none
  /// </summary>
  int32_t pause_at_frame = -1;

  /// <summary>
  /// Whether to pause at the last movie frame
  /// </summary>
  int32_t pause_at_last_frame;

  /// <summary>
  /// Whether VCR operates in read-only mode
  /// </summary>
  int32_t vcr_readonly;

  /// <summary>
  /// Whether VCR creates movie backups
  /// </summary>
  int32_t vcr_backups = 1;

  /// <summary>
  /// Whether mupen will avoid showing modals and other elements which require
  /// user interaction
  /// </summary>
  int32_t silent_mode = 0;

  /// <summary>
  /// The maximum amount of VIs allowed to be generated since the last input
  /// poll before a warning dialog is shown 0 - no warning
  /// </summary>
  int32_t max_lag = 480;

  /// <summary>
  /// The current seeker input value
  /// </summary>
  std::wstring seeker_value;
} t_config;
#pragma pack(pop)

extern t_config g_config;
extern const t_config g_default_config;
extern std::vector<t_hotkey *> g_config_hotkeys;

/**
 * \brief Initializes the config subsystem
 */
void init_config();

/**
 * \brief Saves the current config state to the config file
 */
void save_config();

/**
 * \brief Restores the config state from the config file
 */
void load_config();
