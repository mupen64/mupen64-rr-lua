/***************************************************************************
                                                  main_win.cpp  -  description
                                                         -------------------
        copyright C) 2003    : ShadowPrince (shadow@emulation64.com)
        modifications        : linker (linker@mail.bg)
        mupen64 author       : hacktarux (hacktarux@yahoo.fr)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "Main.h"

#include "Commandline.h"
#include "Loggers.h"
#include "features/Cheats.h"
#include "features/CoreDbg.h"
#include "features/CrashHelper.h"
#include "features/Dispatcher.h"
#include "features/MGECompositor.h"
#include "features/MovieDialog.h"
#include "features/PianoRoll.h"
#include "features/RomBrowser.hpp"
#include "features/Runner.h"
#include "features/Seeker.h"
#include "features/Statusbar.hpp"
#include "shared/AsyncExecutor.h"
#include "shared/services/IOService.h"
#include "shared/services/LoggingService.h"
#include "view/helpers/IOHelpers.h"
#include "wrapper/PersistentPathDialog.h"
#include <Shlwapi.h>
#include <Windows.h>
#include <commctrl.h>
#include <core/memory/memory.h>
#include <core/memory/pif.h>
#include <core/memory/savestates.h>
#include <core/r4300/Plugin.hpp>
#include <core/r4300/r4300.h>
#include <core/r4300/rom.h>
#include <core/r4300/timers.h>
#include <core/r4300/tracelog.h>
#include <core/r4300/vcr.h>
#include <filesystem>
#include <format>
#include <lib/spdlog/sinks/basic_file_sink.h>
#include <lib/spdlog/spdlog.h>
#include <shared/Config.hpp>
#include <shared/Messenger.h>
#include <shared/helpers/StlExtensions.h>
#include <shared/services/FrontendService.h>
#include <shared/services/LuaService.h>
#include <strsafe.h>
#include <variant>
#include <view/capture/EncodingManager.h>
#include <view/gui/features/ConfigDialog.h>
#include <view/helpers/MathHelpers.h>
#include <view/helpers/WinHelpers.h>
#include <view/lua/LuaConsole.h>
#include <view/resource.h>

// Throwaway actions which can be spammed get keys as to not clog up the async
// executor queue
#define ASYNC_KEY_CLOSE_ROM (1)
#define ASYNC_KEY_START_ROM (2)
#define ASYNC_KEY_RESET_ROM (3)
#define ASYNC_KEY_PLAY_MOVIE (4)

DWORD g_ui_thread_id;
HWND g_hwnd_plug;
UINT g_update_screen_timer;
HWND g_main_hwnd;
HMENU g_main_menu;
HMENU g_recent_roms_menu;
HMENU g_recent_movies_menu;
HMENU g_recent_lua_menu;
HINSTANCE g_app_instance;
std::filesystem::path g_app_path;
std::shared_ptr<Dispatcher> g_main_window_dispatcher;

int g_last_wheel_delta = 0;
bool g_paused_before_menu;
bool g_paused_before_focus;
bool g_in_menu_loop;
bool g_vis_since_input_poll_warning_dismissed;
bool g_emu_starting;
bool g_fast_forward;

ULONG_PTR gdi_plus_token;

/**
 * \brief List of lua environment map keys running before emulation stopped
 */
std::vector<HWND> g_previously_running_luas;

constexpr auto WND_CLASS = L"myWindowClass";

const std::map<Action, int> ACTION_ID_MAP = {
    {Action::FastforwardOn, IDM_FASTFORWARD_ON},
    {Action::FastforwardOff, IDM_FASTFORWARD_OFF},
    {Action::GamesharkOn, IDM_GS_ON},
    {Action::GamesharkOff, IDM_GS_OFF},
    {Action::SpeedDown, IDC_DECREASE_MODIFIER},
    {Action::SpeedUp, IDC_INCREASE_MODIFIER},
    {Action::FrameAdvance, IDM_FRAMEADVANCE},
    {Action::Pause, IDM_PAUSE},
    {Action::ToggleReadOnly, IDM_VCR_READONLY},
    {Action::ToggleMovieLoop, IDM_LOOP_MOVIE},
    {Action::StartMoviePlayback, IDM_START_MOVIE_PLAYBACK},
    {Action::StartMovieRecording, IDM_START_MOVIE_RECORDING},
    {Action::StopMovie, IDM_STOP_MOVIE},
    {Action::TakeScreenshot, IDM_SCREENSHOT},
    {Action::PlayLatestMovie, IDM_PLAY_LATEST_MOVIE},
    {Action::LoadLatestScript, IDM_LOAD_LATEST_LUA},
    {Action::NewLua, IDM_LOAD_LUA},
    {Action::CloseAllLua, IDM_CLOSE_ALL_LUA},
    {Action::LoadRom, IDM_LOAD_ROM},
    {Action::CloseRom, IDM_CLOSE_ROM},
    {Action::ResetRom, IDM_RESET_ROM},
    {Action::LoadLatestRom, IDM_LOAD_LATEST_ROM},
    {Action::Fullscreen, IDM_FULLSCREEN},
    {Action::Settings, IDM_SETTINGS},
    {Action::ToggleStatusbar, IDM_STATUSBAR},
    {Action::RefreshRomBrowser, IDM_REFRESH_ROMBROWSER},
    {Action::OpenSeeker, IDM_SEEKER},
    {Action::OpenRunner, IDM_RUNNER},
    {Action::OpenPianoRoll, IDM_PIANO_ROLL},
    {Action::OpenCheats, IDM_CHEATS},
    {Action::SaveSlot, IDM_SAVE_SLOT},
    {Action::LoadSlot, IDM_LOAD_SLOT},
    {Action::SaveAs, IDM_SAVE_STATE_AS},
    {Action::LoadAs, IDM_LOAD_STATE_AS},
    {Action::UndoLoadState, IDM_UNDO_LOAD_STATE},
    {Action::SaveSlot1, (ID_SAVE_1 - 1) + 1},
    {Action::SaveSlot2, (ID_SAVE_1 - 1) + 2},
    {Action::SaveSlot3, (ID_SAVE_1 - 1) + 3},
    {Action::SaveSlot4, (ID_SAVE_1 - 1) + 4},
    {Action::SaveSlot5, (ID_SAVE_1 - 1) + 5},
    {Action::SaveSlot6, (ID_SAVE_1 - 1) + 6},
    {Action::SaveSlot7, (ID_SAVE_1 - 1) + 7},
    {Action::SaveSlot8, (ID_SAVE_1 - 1) + 8},
    {Action::SaveSlot9, (ID_SAVE_1 - 1) + 9},
    {Action::SaveSlot10, (ID_SAVE_1 - 1) + 10},
    {Action::LoadSlot1, (ID_LOAD_1 - 1) + 1},
    {Action::LoadSlot2, (ID_LOAD_1 - 1) + 2},
    {Action::LoadSlot3, (ID_LOAD_1 - 1) + 3},
    {Action::LoadSlot4, (ID_LOAD_1 - 1) + 4},
    {Action::LoadSlot5, (ID_LOAD_1 - 1) + 5},
    {Action::LoadSlot6, (ID_LOAD_1 - 1) + 6},
    {Action::LoadSlot7, (ID_LOAD_1 - 1) + 7},
    {Action::LoadSlot8, (ID_LOAD_1 - 1) + 8},
    {Action::LoadSlot9, (ID_LOAD_1 - 1) + 9},
    {Action::LoadSlot10, (ID_LOAD_1 - 1) + 10},
    {Action::SelectSlot1, (IDM_SELECT_1 - 1) + 1},
    {Action::SelectSlot2, (IDM_SELECT_1 - 1) + 2},
    {Action::SelectSlot3, (IDM_SELECT_1 - 1) + 3},
    {Action::SelectSlot4, (IDM_SELECT_1 - 1) + 4},
    {Action::SelectSlot5, (IDM_SELECT_1 - 1) + 5},
    {Action::SelectSlot6, (IDM_SELECT_1 - 1) + 6},
    {Action::SelectSlot7, (IDM_SELECT_1 - 1) + 7},
    {Action::SelectSlot8, (IDM_SELECT_1 - 1) + 8},
    {Action::SelectSlot9, (IDM_SELECT_1 - 1) + 9},
    {Action::SelectSlot10, (IDM_SELECT_1 - 1) + 10},
};

/// Prompts the user to change their plugin selection.
static void prompt_plugin_change() {
  auto result = FrontendService::show_multiple_choice_dialog(
      {L"Choose Default Plugins", L"Change Plugins", L"Cancel"},
      L"One or more plugins couldn't be loaded.\r\nHow would you like to "
      L"proceed?",
      L"Core", FrontendService::DialogType::Error);

  if (result == 0) {
    auto plugin_discovery_result = do_plugin_discovery();

    auto first_video_plugin = std::ranges::find_if(
        plugin_discovery_result.plugins,
        [](const auto &plugin) { return plugin->type() == PluginType::Video; });

    auto first_audio_plugin = std::ranges::find_if(
        plugin_discovery_result.plugins,
        [](const auto &plugin) { return plugin->type() == PluginType::Audio; });

    auto first_input_plugin = std::ranges::find_if(
        plugin_discovery_result.plugins,
        [](const auto &plugin) { return plugin->type() == PluginType::Input; });

    auto first_rsp_plugin = std::ranges::find_if(
        plugin_discovery_result.plugins,
        [](const auto &plugin) { return plugin->type() == PluginType::RSP; });

    if (first_video_plugin != plugin_discovery_result.plugins.end()) {
      g_config.selected_video_plugin = first_video_plugin->get()->path();
    }

    if (first_audio_plugin != plugin_discovery_result.plugins.end()) {
      g_config.selected_audio_plugin = first_audio_plugin->get()->path();
    }

    if (first_input_plugin != plugin_discovery_result.plugins.end()) {
      g_config.selected_input_plugin = first_input_plugin->get()->path();
    }

    if (first_rsp_plugin != plugin_discovery_result.plugins.end()) {
      g_config.selected_rsp_plugin = first_rsp_plugin->get()->path();
    }

    return;
  }

  if (result == 1) {
    g_config.settings_tab = 0;
    SendMessage(g_main_hwnd, WM_COMMAND, MAKEWPARAM(IDM_SETTINGS, 0), 0);
  }
}

bool show_error_dialog_for_result(const CoreResult result, void *hwnd) {
  if (result == CoreResult::Ok || result == CoreResult::ST_Cancelled ||
      result == CoreResult::VCR_Cancelled) {
    return false;
  }

  g_view_logger->error("[View] show_error_dialog_for_result: CoreType::{}",
                       static_cast<int32_t>(result));

  std::wstring module;
  std::wstring error;

  switch (result) {
#pragma region VCR
  case CoreResult::VCR_InvalidFormat:
    module = L"VCR";
    error = L"The provided data has an invalid format.";
    break;
  case CoreResult::VCR_BadFile:
    module = L"VCR";
    error = L"The provided file is inaccessible or does not exist.";
    break;
  case CoreResult::VCR_InvalidControllers:
    module = L"VCR";
    error = L"The controller configuration is invalid.";
    break;
  case CoreResult::VCR_InvalidSavestate:
    module = L"VCR";
    error = L"The movie's savestate is missing or invalid.";
    break;
  case CoreResult::VCR_InvalidFrame:
    module = L"VCR";
    error = L"The resulting frame is outside the bounds of the movie.";
    break;
  case CoreResult::VCR_NoMatchingRom:
    module = L"VCR";
    error = L"There is no rom which matches this movie.";
    break;
  case CoreResult::VCR_Busy:
    module = L"VCR";
    error = L"The VCR engine is busy.";
    break;
  case CoreResult::VCR_Idle:
    module = L"VCR";
    error = L"The VCR engine is idle, but must be active to complete this "
            L"operation.";
    break;
  case CoreResult::VCR_NotFromThisMovie:
    module = L"VCR";
    error =
        L"The provided freeze buffer is not from the currently active movie.";
    break;
  case CoreResult::VCR_InvalidVersion:
    module = L"VCR";
    error = L"The movie's version is invalid.";
    break;
  case CoreResult::VCR_InvalidExtendedVersion:
    module = L"VCR";
    error = L"The movie's extended version is invalid.";
    break;
  case CoreResult::VCR_NeedsPlaybackOrRecording:
    module = L"VCR";
    error = L"The operation requires a playback or recording task.";
    break;
  case CoreResult::VCR_InvalidStartType:
    module = L"VCR";
    error = L"The provided start type is invalid.";
    break;
  case CoreResult::VCR_WarpModifyAlreadyRunning:
    module = L"VCR";
    error = L"Another warp modify operation is already running.";
    break;
  case CoreResult::VCR_WarpModifyNeedsRecordingTask:
    module = L"VCR";
    error = L"Warp modifications can only be performed during recording.";
    break;
  case CoreResult::VCR_WarpModifyEmptyInputBuffer:
    module = L"VCR";
    error = L"The provided input buffer is empty.";
    break;
  case CoreResult::VCR_SeekAlreadyRunning:
    module = L"VCR";
    error = L"Another seek operation is already running.";
    break;
  case CoreResult::VCR_SeekSavestateLoadFailed:
    module = L"VCR";
    error = L"The seek operation could not be initiated due to a savestate not "
            L"being loaded successfully.";
    break;
  case CoreResult::VCR_SeekSavestateIntervalZero:
    module = L"VCR";
    error = L"The seek operation can't be initiated because the seek savestate "
            L"interval is 0.";
    break;
#pragma endregion
#pragma region VR
  case CoreResult::VR_NoMatchingRom:
    module = L"Core";
    error = L"The ROM couldn't be loaded.\r\nCouldn't find an appropriate ROM.";
    break;
  case CoreResult::VR_PluginError:
    module = L"Core";
    prompt_plugin_change();
    break;
  case CoreResult::VR_RomInvalid:
    module = L"Core";
    error = L"The ROM couldn't be loaded.\r\nVerify that the ROM is a valid "
            L"N64 ROM.";
    break;
  case CoreResult::VR_FileOpenFailed:
    module = L"Core";
    error = L"Failed to open streams to core files.\r\nVerify that Mupen is "
            L"allowed disk access.";
    break;
#pragma endregion
  default:
    module = L"Unknown";
    error = L"Unknown error.";
    return true;
  }

  if (!error.empty()) {
    const auto title =
        std::format(L"{} Error {}", module, static_cast<int32_t>(result));
    FrontendService::show_dialog(error.c_str(), title.c_str(),
                                 FrontendService::DialogType::Error);
  }

  return true;
}

void set_menu_accelerator(int element_id, const wchar_t *acc) {
  wchar_t string[256] = {0};
  GetMenuString(GetMenu(g_main_hwnd), element_id, string, std::size(string),
                MF_BYCOMMAND);

  auto tab = wcschr(string, '\t');
  if (tab)
    *tab = '\0';
  if (StrCmp(acc, L""))
    wsprintf(string, L"%s\t%s", string, acc);

  ModifyMenu(GetMenu(g_main_hwnd), element_id, MF_BYCOMMAND | MF_STRING,
             element_id, string);
}

void set_hotkey_menu_accelerators(t_hotkey *hotkey, int menu_item_id) {
  const auto hotkey_str = hotkey_to_string(hotkey);
  set_menu_accelerator(menu_item_id,
                       hotkey_str == L"(nothing)" ? L"" : hotkey_str.c_str());
}

void SetDlgItemHotkeyAndMenu(HWND hwnd, int idc, t_hotkey *hotkey,
                             int menuItemID) {
  const auto hotkey_str = hotkey_to_string(hotkey);
  SetDlgItemText(hwnd, idc, hotkey_str.c_str());

  set_menu_accelerator(menuItemID,
                       hotkey_str == L"(nothing)" ? L"" : hotkey_str.c_str());
}

const wchar_t *get_input_text() {
  static wchar_t text[1024]{};
  memset(text, 0, sizeof(text));

  BUTTONS b = LuaService::get_last_controller_data(0);
  wsprintf(text, L"(%d, %d) ", b.Y_AXIS, b.X_AXIS);
  if (b.START_BUTTON)
    lstrcatW(text, L"S");
  if (b.Z_TRIG)
    lstrcatW(text, L"Z");
  if (b.A_BUTTON)
    lstrcatW(text, L"A");
  if (b.B_BUTTON)
    lstrcatW(text, L"B");
  if (b.L_TRIG)
    lstrcatW(text, L"L");
  if (b.R_TRIG)
    lstrcatW(text, L"R");
  if (b.U_CBUTTON || b.D_CBUTTON || b.L_CBUTTON || b.R_CBUTTON) {
    lstrcatW(text, L" C");
    if (b.U_CBUTTON)
      lstrcatW(text, L"^");
    if (b.D_CBUTTON)
      lstrcatW(text, L"v");
    if (b.L_CBUTTON)
      lstrcatW(text, L"<");
    if (b.R_CBUTTON)
      lstrcatW(text, L">");
  }
  if (b.U_DPAD || b.D_DPAD || b.L_DPAD || b.R_DPAD) {
    lstrcatW(text, L"D");
    if (b.U_DPAD)
      lstrcatW(text, L"^");
    if (b.D_DPAD)
      lstrcatW(text, L"v");
    if (b.L_DPAD)
      lstrcatW(text, L"<");
    if (b.R_DPAD)
      lstrcatW(text, L">");
  }
  return text;
}

const wchar_t *get_status_text() {
  static wchar_t text[1024]{};
  memset(text, 0, sizeof(text));

  const auto index_adjustment = g_config.vcr_0_index ? 1 : 0;
  const auto current_sample = VCR::get_seek_completion().first;
  const auto current_vi = VCR::get_current_vi();
  const auto is_before_start = static_cast<int64_t>(current_sample) -
                                   static_cast<int64_t>(index_adjustment) <
                               0;

  if (VCR::get_warp_modify_status() == e_warp_modify_status::warping) {
    StringCbPrintfW(text, sizeof(text), L"Warping (%.2f%%)",
                    (double)current_sample / (double)VCR::get_length_samples() *
                        100.0);
    return text;
  }

  if (VCR::get_task() == e_task::recording) {
    if (is_before_start) {
      memset(text, 0, sizeof(text));
    } else {
      wsprintfW(text, L"%d (%d) ", current_vi,
                current_sample - index_adjustment);
    }
  }

  if (VCR::get_task() == e_task::playback) {
    if (is_before_start) {
      memset(text, 0, sizeof(text));
    } else {
      wsprintfW(text, L"%d / %d (%d / %d) ", current_vi, VCR::get_length_vis(),
                current_sample - index_adjustment, VCR::get_length_samples());
    }
  }

  return text;
}

/**
 * \brief Converts a config action to its respective menu ID
 * \param action The action to convert
 * \returns The converted ID, or 0 if no match is found.
 * \remark In case of some toggle actions, the IDs dont map to a menu item, but
 * only to an identifier which is handled in WM_COMMAND
 */
int config_action_to_menu_id(Action action) {
  if (!ACTION_ID_MAP.contains(action)) {
    g_view_logger->info("[View] No menu ID found for action {}",
                        static_cast<int>(action));
    return 0;
  }
  return ACTION_ID_MAP.at(action);
}

std::wstring hotkey_to_string(const t_hotkey *hotkey) {
  char buf[260]{};
  const int k = hotkey->key;

  if (!hotkey->ctrl && !hotkey->shift && !hotkey->alt && !hotkey->key) {
    return L"(nothing)";
  }

  if (hotkey->ctrl)
    strcat(buf, "Ctrl ");
  if (hotkey->shift)
    strcat(buf, "Shift ");
  if (hotkey->alt)
    strcat(buf, "Alt ");
  if (k) {
    char buf2[64] = {0};
    if ((k >= 0x30 && k <= 0x39) || (k >= 0x41 && k <= 0x5A))
      sprintf(buf2, "%c", static_cast<char>(k));
    else if ((k >= VK_F1 && k <= VK_F24))
      sprintf(buf2, "F%d", k - (VK_F1 - 1));
    else if ((k >= VK_NUMPAD0 && k <= VK_NUMPAD9))
      sprintf(buf2, "Num%d", k - VK_NUMPAD0);
    else
      switch (k) {
      case VK_SPACE:
        strcpy(buf2, "Space");
        break;
      case VK_BACK:
        strcpy(buf2, "Backspace");
        break;
      case VK_TAB:
        strcpy(buf2, "Tab");
        break;
      case VK_CLEAR:
        strcpy(buf2, "Clear");
        break;
      case VK_RETURN:
        strcpy(buf2, "Enter");
        break;
      case VK_PAUSE:
        strcpy(buf2, "Pause");
        break;
      case VK_CAPITAL:
        strcpy(buf2, "Caps");
        break;
      case VK_PRIOR:
        strcpy(buf2, "PageUp");
        break;
      case VK_NEXT:
        strcpy(buf2, "PageDn");
        break;
      case VK_END:
        strcpy(buf2, "End");
        break;
      case VK_HOME:
        strcpy(buf2, "Home");
        break;
      case VK_LEFT:
        strcpy(buf2, "Left");
        break;
      case VK_UP:
        strcpy(buf2, "Up");
        break;
      case VK_RIGHT:
        strcpy(buf2, "Right");
        break;
      case VK_DOWN:
        strcpy(buf2, "Down");
        break;
      case VK_SELECT:
        strcpy(buf2, "Select");
        break;
      case VK_PRINT:
        strcpy(buf2, "Print");
        break;
      case VK_SNAPSHOT:
        strcpy(buf2, "PrintScrn");
        break;
      case VK_INSERT:
        strcpy(buf2, "Insert");
        break;
      case VK_DELETE:
        strcpy(buf2, "Delete");
        break;
      case VK_HELP:
        strcpy(buf2, "Help");
        break;
      case VK_MULTIPLY:
        strcpy(buf2, "Num*");
        break;
      case VK_ADD:
        strcpy(buf2, "Num+");
        break;
      case VK_SUBTRACT:
        strcpy(buf2, "Num-");
        break;
      case VK_DECIMAL:
        strcpy(buf2, "Num.");
        break;
      case VK_DIVIDE:
        strcpy(buf2, "Num/");
        break;
      case VK_NUMLOCK:
        strcpy(buf2, "NumLock");
        break;
      case VK_SCROLL:
        strcpy(buf2, "ScrollLock");
        break;
      case /*VK_OEM_PLUS*/ 0xBB:
        strcpy(buf2, "=+");
        break;
      case /*VK_OEM_MINUS*/ 0xBD:
        strcpy(buf2, "-_");
        break;
      case /*VK_OEM_COMMA*/ 0xBC:
        strcpy(buf2, ",");
        break;
      case /*VK_OEM_PERIOD*/ 0xBE:
        strcpy(buf2, ".");
        break;
      case VK_OEM_7:
        strcpy(buf2, "'\"");
        break;
      case VK_OEM_6:
        strcpy(buf2, "]}");
        break;
      case VK_OEM_5:
        strcpy(buf2, "\\|");
        break;
      case VK_OEM_4:
        strcpy(buf2, "[{");
        break;
      case VK_OEM_3:
        strcpy(buf2, "`~");
        break;
      case VK_OEM_2:
        strcpy(buf2, "/?");
        break;
      case VK_OEM_1:
        strcpy(buf2, ";:");
        break;
      default:
        sprintf(buf2, "(%d)", k);
        break;
      }
    strcat(buf, buf2);
  }
  // TODO: Port to Unicode properly
  return string_to_wstring(buf);
}

namespace Recent {
void build(std::vector<std::wstring> &vec, int first_menu_id, HMENU parent_menu,
           bool reset = false) {
  assert(is_on_gui_thread());

  // First 3 items aren't dynamic and shouldn't be deleted
  constexpr int FIXED_ITEM_COUNT = 3;

  const int child_count = GetMenuItemCount(parent_menu) - FIXED_ITEM_COUNT;

  for (int i = 0; i < child_count; ++i) {
    DeleteMenu(parent_menu, FIXED_ITEM_COUNT + i, MF_BYPOSITION);
  }

  if (reset) {
    vec.clear();
    return;
  }

  MENUITEMINFO menu_info = {0};
  menu_info.cbSize = sizeof(MENUITEMINFO);
  menu_info.fMask = MIIM_TYPE | MIIM_ID;
  menu_info.fType = MFT_STRING;
  menu_info.fState = MFS_ENABLED;

  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i].empty()) {
      continue;
    }
    menu_info.dwTypeData = vec[i].data();
    menu_info.cch = vec[i].size();
    menu_info.wID = first_menu_id + i;
    InsertMenuItem(parent_menu, i + 3, TRUE, &menu_info);
  }
}

void add(std::vector<std::wstring> &vec, std::wstring val, bool frozen,
         int first_menu_id, HMENU parent_menu) {
  assert(is_on_gui_thread());

  if (frozen) {
    return;
  }
  if (vec.size() > 5) {
    vec.pop_back();
  }
  std::erase(vec, val);
  vec.insert(vec.begin(), val);
  build(vec, first_menu_id, parent_menu);
}

std::wstring element_at(std::vector<std::wstring> vec, int first_menu_id,
                        int menu_id) {
  const int index = menu_id - first_menu_id;
  if (index >= 0 && index < vec.size()) {
    return vec[index];
  }
  return L"";
}
} // namespace Recent

std::filesystem::path get_screenshots_directory() {
  if (g_config.is_default_screenshots_directory_used) {
    return g_app_path / L"screenshots\\";
  }
  return g_config.screenshots_directory;
}

std::filesystem::path get_plugins_directory() {
  if (g_config.is_default_plugins_directory_used) {
    return g_app_path / L"plugin\\";
  }
  return g_config.plugins_directory;
}

void update_titlebar() {
  std::wstring text = MUPEN_VERSION;

  if (g_emu_starting) {
    text += L" - Starting...";
  }

  if (emu_launched) {
    text += std::format(
        L" - {}", string_to_wstring(reinterpret_cast<char *>(ROM_HEADER.nom)));
  }

  if (VCR::get_task() != e_task::idle) {
    wchar_t movie_filename[MAX_PATH] = {0};
    _wsplitpath(VCR::get_path().wstring().c_str(), nullptr, nullptr,
                movie_filename, nullptr);
    text += std::format(L" - {}", movie_filename);
  }

  if (EncodingManager::is_capturing()) {
    text += std::format(
        L" - {}", EncodingManager::get_current_path().filename().wstring());
  }

  SetWindowText(g_main_hwnd, text.c_str());
}

/**
 * \brief Computes the average rate of entries in the time queue per second
 * (e.g.: FPS from frame deltas) \param times A circular buffer of deltas
 * \return The average rate per second from the delta in the queue
 */
static double
get_rate_per_second_from_deltas(const std::span<timer_delta> &times) {
  size_t count = 0;
  double sum = 0.0;
  for (const auto &time : times) {
    if (time.count() > 0.0) {
      sum += time.count() / 1000000.0;
      count++;
    }
  }
  return count > 0 ? 1000.0 / (sum / count) : 0.0;
}

#pragma region Change notifications

void on_script_started(std::any data) {
  g_main_window_dispatcher->invoke([=] {
    auto value = std::any_cast<std::filesystem::path>(data);
    Recent::add(g_config.recent_lua_script_paths, value.wstring(),
                g_config.is_recent_scripts_frozen, ID_LUA_RECENT,
                g_recent_lua_menu);
  });
}

void on_task_changed(std::any data) {
  g_main_window_dispatcher->invoke([=] {
    auto value = std::any_cast<e_task>(data);
    static auto previous_value = value;
    if (!task_is_recording(value) && task_is_recording(previous_value)) {
      Statusbar::post(L"Recording stopped");
    }
    if (!task_is_playback(value) && task_is_playback(previous_value)) {
      Statusbar::post(L"Playback stopped");
    }

    if ((task_is_recording(value) && !task_is_recording(previous_value)) ||
        task_is_playback(value) && !task_is_playback(previous_value) &&
            !VCR::get_path().empty()) {
      Recent::add(g_config.recent_movie_paths, VCR::get_path().wstring(),
                  g_config.is_recent_movie_paths_frozen, ID_RECENTMOVIES_FIRST,
                  g_recent_movies_menu);
    }

    update_titlebar();
    SendMessage(g_main_hwnd, WM_INITMENU, 0, 0);
    previous_value = value;
  });
}

void on_emu_stopping(std::any) {
  // Remember all running lua scripts' HWNDs
  for (const auto [key, _] : g_hwnd_lua_map) {
    g_previously_running_luas.push_back(key);
  }
  g_main_window_dispatcher->invoke(stop_all_scripts);
}

void on_emu_launched_changed(std::any data) {
  g_main_window_dispatcher->invoke([=] {
    auto value = std::any_cast<bool>(data);
    static auto previous_value = value;

    const auto window_style = GetWindowLong(g_main_hwnd, GWL_STYLE);
    if (value) {
      SetWindowLong(g_main_hwnd, GWL_STYLE,
                    window_style & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
    } else {
      SetWindowLong(g_main_hwnd, GWL_STYLE,
                    window_style | WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    update_titlebar();
    // Some menu items, like movie ones, depend on both this and vcr task
    on_task_changed(VCR::get_task());

    // Reset and restore view stuff when emulation starts
    if (value) {
      g_vis_since_input_poll_warning_dismissed = false;

      Recent::add(g_config.recent_rom_paths, get_rom_path().wstring(),
                  g_config.is_recent_rom_paths_frozen, ID_RECENTROMS_FIRST,
                  g_recent_roms_menu);

      for (const HWND hwnd : g_previously_running_luas) {
        // click start button
        SendMessage(hwnd, WM_COMMAND,
                    MAKEWPARAM(IDC_BUTTON_LUASTATE, BN_CLICKED),
                    (LPARAM)GetDlgItem(hwnd, IDC_BUTTON_LUASTATE));
      }

      g_previously_running_luas.clear();
    }

    if (!value && previous_value) {
      g_view_logger->info("[View] Restoring window size to {}x{}...",
                          g_config.window_width, g_config.window_height);
      SetWindowPos(g_main_hwnd, nullptr, 0, 0, g_config.window_width,
                   g_config.window_height, SWP_NOMOVE);
    }

    SendMessage(g_main_hwnd, WM_INITMENU, 0, 0);
    previous_value = value;
  });
}

void on_capturing_changed(std::any data) {
  g_main_window_dispatcher->invoke([=] {
    auto value = std::any_cast<bool>(data);

    if (value) {
      SetWindowLong(g_main_hwnd, GWL_STYLE,
                    GetWindowLong(g_main_hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
      // NOTE: WS_EX_LAYERED fixes BitBlt'ing from the window when its
      // off-screen, as it wouldnt redraw otherwise (relevant for Window capture
      // mode)
      SetWindowLong(g_main_hwnd, GWL_EXSTYLE,
                    GetWindowLong(g_main_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    } else {
      SetWindowPos(g_main_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
      SetWindowLong(g_main_hwnd, GWL_STYLE,
                    GetWindowLong(g_main_hwnd, GWL_STYLE) | WS_MINIMIZEBOX);
      SetWindowLong(g_main_hwnd, GWL_EXSTYLE,
                    GetWindowLong(g_main_hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    }

    update_titlebar();
    SendMessage(g_main_hwnd, WM_INITMENU, 0, 0);
  });
}

void on_speed_modifier_changed(std::any data) {
  auto value = std::any_cast<int32_t>(data);
  Statusbar::post(std::format(L"Speed limit: {}%", value));
}

void on_emu_paused_changed(std::any data) {
  frame_changed = true;
  SendMessage(g_main_hwnd, WM_INITMENU, 0, 0);
}

void on_vis_since_input_poll_exceeded(std::any) {
  if (g_vis_since_input_poll_warning_dismissed) {
    return;
  }

  if (g_config.silent_mode ||
      FrontendService::show_ask_dialog(
          L"An unusual execution pattern was detected. Continuing might leave "
          L"the emulator in an unusable state.\r\nWould you like to terminate "
          L"emulation?",
          L"Warning", true)) {
    // TODO: Send IDM_CLOSE_ROM instead... probably better :P
    AsyncExecutor::invoke_async([] {
      const auto result = vr_close_rom();
      show_error_dialog_for_result(result);
    });
  }
  g_vis_since_input_poll_warning_dismissed = true;
}

void on_movie_loop_changed(std::any data) {
  auto value = std::any_cast<bool>(data);
  Statusbar::post(value ? L"Movies restart after ending"
                        : L"Movies stop after ending");
  SendMessage(g_main_hwnd, WM_INITMENU, 0, 0);
}

void on_fullscreen_changed(std::any data) {
  g_main_window_dispatcher->invoke([=] {
    auto value = std::any_cast<bool>(data);
    ShowCursor(!value);
    SendMessage(g_main_hwnd, WM_INITMENU, 0, 0);
  });
}

void on_config_loaded(std::any) {
  for (auto hotkey : g_config_hotkeys) {
    // Only set accelerator if hotkey has a down command and the command is
    // valid menu item identifier
    auto down_cmd = config_action_to_menu_id(hotkey->down_cmd);
    auto state = GetMenuState(GetMenu(g_main_hwnd), down_cmd, MF_BYCOMMAND);
    if (down_cmd && state != -1) {
      set_hotkey_menu_accelerators(hotkey, down_cmd);
    }
  }
  Rombrowser::build();
}

void on_seek_completed(std::any) { LuaService::call_seek_completed(); }

void on_warp_modify_status_changed(std::any data) {
  auto value = std::any_cast<e_warp_modify_status>(data);
  LuaService::call_warp_modify_status_changed(static_cast<int32_t>(value));
}

void update_core_fast_forward(std::any) {
  g_vr_fast_forward = g_fast_forward || VCR::is_seeking() ||
                      g_vr_benchmark_enabled || Cli::wants_fast_forward();
}

BetterEmulationLock::BetterEmulationLock() {
  if (g_in_menu_loop) {
    was_paused = g_paused_before_menu;

    // This fires before WM_EXITMENULOOP (which restores the paused_before_menu
    // state), so we need to trick it...
    g_paused_before_menu = true;
  } else {
    was_paused = emu_paused;
    pause_emu();
  }
}

BetterEmulationLock::~BetterEmulationLock() {
  if (was_paused) {
    pause_emu();
  } else {
    resume_emu();
  }
}

t_window_info get_window_info() {
  t_window_info info;

  RECT client_rect = {};
  GetClientRect(g_main_hwnd, &client_rect);

  // full client dimensions including statusbar
  info.width = client_rect.right - client_rect.left;
  info.height = client_rect.bottom - client_rect.top;

  RECT statusbar_rect = {0};
  if (Statusbar::hwnd())
    GetClientRect(Statusbar::hwnd(), &statusbar_rect);
  info.statusbar_height = statusbar_rect.bottom - statusbar_rect.top;

  // subtract size of toolbar and statusbar from buffer dimensions
  // if video plugin knows about this, whole game screen should be captured.
  // Most of the plugins do.
  info.height -= info.statusbar_height;
  return info;
}

#pragma endregion

bool confirm_user_exit() {
  if (g_config.silent_mode) {
    return true;
  }

  int res = 0;
  int warnings = 0;

  std::wstring final_message;
  if (VCR::get_task() == e_task::recording) {
    final_message.append(L"Movie recording ");
    warnings++;
  }
  if (EncodingManager::is_capturing()) {
    if (warnings > 0) {
      final_message.append(L",");
    }
    final_message.append(L" AVI capture ");
    warnings++;
  }
  if (tracelog::active()) {
    if (warnings > 0) {
      final_message.append(L",");
    }
    final_message.append(L" Trace logging ");
    warnings++;
  }
  final_message.append(L"is running. Are you sure you want to stop emulation?");
  if (warnings > 0)
    res = MessageBox(g_main_hwnd, final_message.c_str(), L"Stop emulation?",
                     MB_YESNO | MB_ICONWARNING);

  return res == IDYES || warnings == 0;
}

bool is_on_gui_thread() { return GetCurrentThreadId() == g_ui_thread_id; }

void ClearButtons() {
  BUTTONS zero = {0};
  for (int i = 0; i < 4; i++) {
    setKeys(i, zero);
  }
}

std::filesystem::path get_app_full_path() {
  wchar_t ret[MAX_PATH] = {0};
  wchar_t drive[_MAX_DRIVE], dirn[_MAX_DIR];
  wchar_t path_buffer[_MAX_DIR];
  GetModuleFileName(nullptr, path_buffer, std::size(path_buffer));
  _wsplitpath(path_buffer, drive, dirn, nullptr, nullptr);
  StrCpy(ret, drive);
  StrCat(ret, dirn);

  return ret;
}

t_plugin_discovery_result do_plugin_discovery() {
  std::vector<std::unique_ptr<Plugin>> plugins;
  const auto files = IOService::get_files_with_extension_in_directory(
      get_plugins_directory(), L"dll");

  std::vector<std::pair<std::filesystem::path, std::wstring>> results;
  for (const auto &file : files) {
    auto [result, plugin] = Plugin::create(file);

    results.emplace_back(file, result);

    if (!result.empty())
      continue;

    plugins.emplace_back(std::move(plugin));
  }

  return t_plugin_discovery_result{
      .plugins = std::move(plugins),
      .results = results,
  };
}

void open_console() {
  AllocConsole();
  FILE *f = 0;
  freopen_s(&f, "CONIN$", "r", stdin);
  freopen_s(&f, "CONOUT$", "w", stdout);
  freopen_s(&f, "CONOUT$", "w", stderr);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam,
                         LPARAM lParam) {
  if (Message == WM_EXECUTE_DISPATCHER) {
    g_main_window_dispatcher->execute();
    return TRUE;
  }

  wchar_t path_buffer[_MAX_PATH]{};

  switch (Message) {
  case WM_DROPFILES: {
    auto drop = (HDROP)wParam;
    wchar_t fname[MAX_PATH] = {0};
    DragQueryFile(drop, 0, fname, std::size(fname));

    std::filesystem::path path = fname;
    std::string extension = to_lower(path.extension().string());

    if (extension == ".n64" || extension == ".z64" || extension == ".v64" ||
        extension == ".rom") {
      AsyncExecutor::invoke_async([path] {
        const auto result = vr_start_rom(path);
        show_error_dialog_for_result(result);
      });
    } else if (extension == ".m64") {
      g_config.vcr_readonly = true;
      Messenger::broadcast(Messenger::Message::ReadonlyChanged,
                           (bool)g_config.vcr_readonly);
      AsyncExecutor::invoke_async([fname] {
        auto result = VCR::start_playback(fname);
        show_error_dialog_for_result(result);
      });
    } else if (extension == ".st" || extension == ".savestate" ||
               extension == ".st0" || extension == ".st1" ||
               extension == ".st2" || extension == ".st3" ||
               extension == ".st4" || extension == ".st5" ||
               extension == ".st6" || extension == ".st7" ||
               extension == ".st8" || extension == ".st9") {
      if (!emu_launched)
        break;
      ++g_vr_wait_before_input_poll;
      AsyncExecutor::invoke_async([=] {
        --g_vr_wait_before_input_poll;
        Savestates::do_file(fname, Savestates::Job::Load);
      });
    } else if (extension == ".lua") {
      lua_create_and_run(path.wstring().c_str());
    }
    break;
  }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN: {
    BOOL hit = FALSE;
    for (t_hotkey *hotkey : g_config_hotkeys) {
      if ((int)wParam == hotkey->key) {
        if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkey->shift &&
            ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == hotkey->ctrl &&
            ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkey->alt) {
          auto down_cmd = config_action_to_menu_id(hotkey->down_cmd);
          // We only want to send it if the corresponding menu item exists and
          // is enabled
          auto state = GetMenuState(g_main_menu, down_cmd, MF_BYCOMMAND);
          if (state != -1 && (state & MF_DISABLED || state & MF_GRAYED)) {
            g_view_logger->info(L"Dismissed {} ({})",
                                hotkey->identifier.c_str(), down_cmd);
            continue;
          }
          g_view_logger->info(L"Sent down {} ({})", hotkey->identifier.c_str(),
                              down_cmd);
          SendMessage(g_main_hwnd, WM_COMMAND, down_cmd, 0);
          hit = TRUE;
        }
      }
    }

    if (keyDown && emu_launched)
      keyDown(wParam, lParam);
    if (!hit)
      return DefWindowProc(hwnd, Message, wParam, lParam);
    break;
  }
  case WM_SYSKEYUP:
  case WM_KEYUP: {
    BOOL hit = FALSE;
    for (t_hotkey *hotkey : g_config_hotkeys) {
      if (hotkey->up_cmd == Action::None) {
        continue;
      }

      if ((int)wParam == hotkey->key) {
        if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkey->shift &&
            ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == hotkey->ctrl &&
            ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkey->alt) {
          auto up_cmd = config_action_to_menu_id(hotkey->up_cmd);
          // We only want to send it if the corresponding menu item exists and
          // is enabled
          auto state = GetMenuState(g_main_menu, up_cmd, MF_BYCOMMAND);
          if (state != -1 && (state & MF_DISABLED || state & MF_GRAYED)) {
            g_view_logger->info(L"Dismissed {} ({})",
                                hotkey->identifier.c_str(), up_cmd);
            continue;
          }
          g_view_logger->info(L"Sent up {} ({})", hotkey->identifier.c_str(),
                              up_cmd);
          SendMessage(g_main_hwnd, WM_COMMAND, up_cmd, 0);
          hit = TRUE;
        }
      }
    }

    if (keyUp && emu_launched)
      keyUp(wParam, lParam);
    if (!hit)
      return DefWindowProc(hwnd, Message, wParam, lParam);
    break;
  }
  case WM_MOUSEWHEEL:
    g_last_wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam);

    // https://github.com/mkdasher/mupen64-rr-lua-/issues/190
    LuaService::call_window_message(hwnd, Message, wParam, lParam);
    break;
  case WM_NOTIFY: {
    LPNMHDR l_header = (LPNMHDR)lParam;

    if (wParam == IDC_ROMLIST) {
      Rombrowser::notify(lParam);
    }
    return 0;
  }
  case WM_MOVE: {
    if (core_executing) {
      moveScreen((int)wParam, lParam);
    }

    if (IsIconic(g_main_hwnd)) {
      // GetWindowRect values are nonsense when minimized
      break;
    }

    RECT rect = {0};
    GetWindowRect(g_main_hwnd, &rect);
    g_config.window_x = rect.left;
    g_config.window_y = rect.top;
    break;
  }
  case WM_SIZE: {
    SendMessage(Statusbar::hwnd(), WM_SIZE, 0, 0);
    RECT rect{};
    GetClientRect(g_main_hwnd, &rect);
    Messenger::broadcast(Messenger::Message::SizeChanged, rect);

    if (core_executing || emu_launched) {
      // We don't need to remember the dimensions set by gfx plugin
      break;
    }

    if (IsIconic(g_main_hwnd)) {
      // GetWindowRect values are nonsense when minimized
      break;
    }

    // Window creation expects the size with nc area, so it's easiest to just
    // use the window rect here
    GetWindowRect(hwnd, &rect);
    g_config.window_width = rect.right - rect.left;
    g_config.window_height = rect.bottom - rect.top;

    break;
  }
  case WM_FOCUS_MAIN_WINDOW:
    SetFocus(g_main_hwnd);
    break;
  case WM_CREATE:
    g_main_window_dispatcher = std::make_unique<Dispatcher>(g_ui_thread_id, [] {
      SendMessage(g_main_hwnd, WM_EXECUTE_DISPATCHER, 0, 0);
    });
    g_main_menu = GetMenu(hwnd);
    GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
    g_update_screen_timer =
        SetTimer(hwnd, NULL,
                 (uint32_t)(1000 / get_primary_monitor_refresh_rate()), NULL);
    MGECompositor::create(hwnd);
    PianoRoll::init();
    configdialog_init();
    return TRUE;
  case WM_DESTROY:
    save_config();
    KillTimer(g_main_hwnd, g_update_screen_timer);
    AsyncExecutor::stop();
    Gdiplus::GdiplusShutdown(gdi_plus_token);
    PostQuitMessage(0);
    break;
  case WM_CLOSE:
    if (confirm_user_exit()) {
      close_all_scripts();
      std::thread([] {
        vr_close_rom(true);
        g_main_window_dispatcher->invoke([] { DestroyWindow(g_main_hwnd); });
      }).detach();
      break;
    }
    return 0;
  case WM_TIMER: {
    static std::chrono::high_resolution_clock::time_point
        last_statusbar_update = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point time =
        std::chrono::high_resolution_clock::now();

    if (frame_changed) {
      Statusbar::post(get_input_text(), Statusbar::Section::Input);

      if (EncodingManager::is_capturing()) {
        if (VCR::get_task() == e_task::idle) {
          Statusbar::post(
              std::format(L"{}", EncodingManager::get_video_frame()),
              Statusbar::Section::VCR);
        } else {
          Statusbar::post(std::format(L"{}({})", get_status_text(),
                                      EncodingManager::get_video_frame()),
                          Statusbar::Section::VCR);
        }
      } else {
        Statusbar::post(get_status_text(), Statusbar::Section::VCR);
      }

      frame_changed = false;
    }

    // NOTE: We don't invalidate the controls in their WM_PAINT, since that
    // generates too many WM_PAINTs and fills up the message queue Instead, we
    // invalidate them driven by not so high-freq heartbeat like previously.
    for (auto map : g_hwnd_lua_map) {
      map.second->invalidate_visuals();
    }

    // We throttle FPS and VI/s visual updates to 1 per second, so no unstable
    // values are displayed
    if (time - last_statusbar_update > std::chrono::seconds(1)) {
      g_frame_deltas_mutex.lock();
      auto fps = get_rate_per_second_from_deltas(g_frame_deltas);
      g_frame_deltas_mutex.unlock();

      g_vi_deltas_mutex.lock();
      auto vis = get_rate_per_second_from_deltas(g_vi_deltas);
      g_vi_deltas_mutex.unlock();

      Statusbar::post(std::format(L"FPS: {:.1f}", fps),
                      Statusbar::Section::FPS);
      Statusbar::post(std::format(L"VI/s: {:.1f}", vis),
                      Statusbar::Section::VIs);

      last_statusbar_update = time;
    }
  } break;
  case WM_WINDOWPOSCHANGING: // allow gfx plugin to set arbitrary size
    return 0;
  case WM_GETMINMAXINFO: {
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    lpMMI->ptMinTrackSize.x = MIN_WINDOW_W;
    lpMMI->ptMinTrackSize.y = MIN_WINDOW_H;
    // this might break small res with gfx plugin!!!
  } break;
  case WM_INITMENU: {
    EnableMenuItem(g_main_menu, IDM_CLOSE_ROM,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_RESET_ROM,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_PAUSE,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_FRAMEADVANCE,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_SCREENSHOT,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_SAVE_SLOT,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_LOAD_SLOT,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_SAVE_STATE_AS,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_LOAD_STATE_AS,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_UNDO_LOAD_STATE,
                   (emu_launched && g_config.st_undo_load) ? MF_ENABLED
                                                           : MF_GRAYED);
    for (int i = IDM_SELECT_1; i < IDM_SELECT_10; ++i) {
      EnableMenuItem(g_main_menu, i, emu_launched ? MF_ENABLED : MF_GRAYED);
    }
    EnableMenuItem(g_main_menu, IDM_FULLSCREEN,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_STATUSBAR,
                   !emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_START_MOVIE_RECORDING,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_STOP_MOVIE,
                   (VCR::get_task() != e_task::idle) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_TRACELOG,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_COREDBG,
                   (emu_launched && g_config.core_type == 2) ? MF_ENABLED
                                                             : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_SEEKER,
                   (emu_launched && VCR::get_task() != e_task::idle)
                       ? MF_ENABLED
                       : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_PIANO_ROLL,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_CHEATS,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_START_CAPTURE,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_START_CAPTURE_PRESET,
                   emu_launched ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(g_main_menu, IDM_STOP_CAPTURE,
                   (emu_launched && EncodingManager::is_capturing())
                       ? MF_ENABLED
                       : MF_GRAYED);

    CheckMenuItem(g_main_menu, IDM_STATUSBAR,
                  g_config.is_statusbar_enabled ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(g_main_menu, IDM_FREEZE_RECENT_ROMS,
                  g_config.is_recent_rom_paths_frozen ? MF_CHECKED
                                                      : MF_UNCHECKED);
    CheckMenuItem(g_main_menu, IDM_FREEZE_RECENT_MOVIES,
                  g_config.is_recent_movie_paths_frozen ? MF_CHECKED
                                                        : MF_UNCHECKED);
    CheckMenuItem(g_main_menu, IDM_FREEZE_RECENT_LUA,
                  g_config.is_recent_scripts_frozen ? MF_CHECKED
                                                    : MF_UNCHECKED);
    CheckMenuItem(g_main_menu, IDM_LOOP_MOVIE,
                  g_config.is_movie_loop_enabled ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(g_main_menu, IDM_VCR_READONLY,
                  g_config.vcr_readonly ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(g_main_menu, IDM_FULLSCREEN,
                  vr_is_fullscreen() ? MF_CHECKED : MF_UNCHECKED);

    for (int i = IDM_SELECT_1; i < IDM_SELECT_10; ++i) {
      CheckMenuItem(g_main_menu, i, MF_UNCHECKED);
    }
    CheckMenuItem(g_main_menu, IDM_SELECT_1 + g_config.st_slot, MF_CHECKED);
  } break;
  case WM_ENTERMENULOOP:
    g_in_menu_loop = true;
    g_paused_before_menu = emu_paused;
    pause_emu();
    break;

  case WM_EXITMENULOOP:
    // This message is sent when we escape the blocking menu loop, including
    // situations where the clicked menu spawns a dialog. In those situations,
    // we would unpause the game here (since this message is sent first), and
    // then pause it again in the menu item message handler. It's almost
    // guaranteed that a game frame will pass between those messages, so we need
    // to wait a bit on another thread before unpausing.
    std::thread([] {
      Sleep(60);
      g_in_menu_loop = false;
      if (g_paused_before_menu) {
        pause_emu();
      } else {
        resume_emu();
      }
    }).detach();
    break;
  case WM_ACTIVATE:
    UpdateWindow(hwnd);

    if (!g_config.is_unfocused_pause_enabled) {
      break;
    }

    switch (LOWORD(wParam)) {
    case WA_ACTIVE:
    case WA_CLICKACTIVE:
      if (!g_paused_before_focus) {
        resume_emu();
      }
      break;

    case WA_INACTIVE:
      g_paused_before_focus = emu_paused;
      pause_emu();
      break;
    default:
      break;
    }
    break;
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDM_VIDEO_SETTINGS:
    case IDM_INPUT_SETTINGS:
    case IDM_AUDIO_SETTINGS:
    case IDM_RSP_SETTINGS: {
      // FIXME: Is it safe to load multiple plugin instances? This assumes they
      // cooperate and dont overwrite eachother's files... It does seem fine
      // tho, since the config dialog is modal and core is paused
      g_hwnd_plug = g_main_hwnd;
      std::unique_ptr<Plugin> plugin;

      switch (LOWORD(wParam)) {
      case IDM_VIDEO_SETTINGS:
        plugin = Plugin::create(g_config.selected_video_plugin).second;
        break;
      case IDM_INPUT_SETTINGS:
        plugin = Plugin::create(g_config.selected_input_plugin).second;
        break;
      case IDM_AUDIO_SETTINGS:
        plugin = Plugin::create(g_config.selected_audio_plugin).second;
        break;
      case IDM_RSP_SETTINGS:
        plugin = Plugin::create(g_config.selected_rsp_plugin).second;
        break;
      }

      if (plugin != nullptr) {
        plugin->config();
      }
    } break;
    case IDM_LOAD_LUA: {
      lua_create();
    } break;

    case IDM_CLOSE_ALL_LUA:
      close_all_scripts();
      break;
    case IDM_DEBUG_WARP_MODIFY: {
      auto inputs = VCR::get_inputs();
      inputs[inputs.size() - 10].A_BUTTON = 1;

      auto result = VCR::begin_warp_modify(inputs);
      show_error_dialog_for_result(result);

      break;
    }
    case IDM_BENCHMARK_MESSENGER: {
      ScopeTimer timer("Messenger", g_view_logger);
      for (int i = 0; i < 10'000'000; ++i) {
        Messenger::broadcast(Messenger::Message::None, 5);
      }
    } break;
    case IDM_BENCHMARK_LUA_CALLBACK: {
      FrontendService::show_dialog(L"Make sure the Lua script is running and "
                                   L"the registered atreset body is empty.",
                                   L"Benchmark Lua Callback",
                                   FrontendService::DialogType::Information);
      ScopeTimer timer("100,000,000x call_reset", g_view_logger);
      for (int i = 0; i < 100'000'000; ++i) {
        LuaService::call_reset();
      }
    } break;
    case IDM_BENCHMARK_CORE_START:
      Core::start_benchmark();
      Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
      break;
    case IDM_BENCHMARK_CORE_STOP: {
      auto fps = Core::stop_benchmark();
      FrontendService::show_dialog(std::format(L"FPS: {:2f}", fps).c_str(),
                                   L"Benchmark Core",
                                   FrontendService::DialogType::Information);
      Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
      break;
    }
    case IDM_TRACELOG: {
      if (tracelog::active()) {
        tracelog::stop();
        ModifyMenu(g_main_menu, IDM_TRACELOG, MF_BYCOMMAND | MF_STRING,
                   IDM_TRACELOG, L"Start &Trace Logger...");
        break;
      }

      auto path =
          show_persistent_save_dialog(L"s_tracelog", g_main_hwnd, L"*.log");

      if (path.empty()) {
        break;
      }

      auto result = MessageBox(
          g_main_hwnd, L"Should the trace log be generated in a binary format?",
          L"Trace Logger", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

      tracelog::start(path, result == IDYES);
      ModifyMenu(g_main_menu, IDM_TRACELOG, MF_BYCOMMAND | MF_STRING,
                 IDM_TRACELOG, L"Stop &Trace Logger");
    } break;
    case IDM_CLOSE_ROM:
      if (!confirm_user_exit())
        break;
      AsyncExecutor::invoke_async(
          [] {
            const auto result = vr_close_rom();
            show_error_dialog_for_result(result);
          },
          ASYNC_KEY_CLOSE_ROM);
      break;
    case IDM_FASTFORWARD_ON:
      g_fast_forward = true;
      Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
      break;
    case IDM_FASTFORWARD_OFF:
      g_fast_forward = false;
      Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
      break;
    case IDM_GS_ON:
      set_gs_button(true);
      break;
    case IDM_GS_OFF:
      set_gs_button(false);
      break;
    case IDM_PAUSE: {
      // FIXME: While this is a beautiful and clean solution, there has to be a
      // better way to handle this We're too close to release to care tho
      if (g_in_menu_loop) {
        if (g_paused_before_menu) {
          resume_emu();
          g_paused_before_menu = false;
          break;
        }
        g_paused_before_menu = true;
        pause_emu();
      } else {
        if (emu_paused) {
          resume_emu();
          break;
        }
        pause_emu();
      }

      break;
    }

    case IDM_FRAMEADVANCE: {
      frame_advancing = 1;
      resume_emu();
    } break;

    case IDM_VCR_READONLY:
      g_config.vcr_readonly ^= true;
      Messenger::broadcast(Messenger::Message::ReadonlyChanged,
                           (bool)g_config.vcr_readonly);
      break;

    case IDM_LOOP_MOVIE:
      g_config.is_movie_loop_enabled ^= true;
      Messenger::broadcast(Messenger::Message::MovieLoopChanged,
                           (bool)g_config.is_movie_loop_enabled);
      break;

    case EMU_PLAY:
      resume_emu();
      break;

    case IDM_RESET_ROM:
      if (g_config.is_reset_recording_enabled &&
          VCR::get_task() == e_task::recording) {
        Messenger::broadcast(Messenger::Message::ResetRequested, nullptr);
        break;
      }

      if (!confirm_user_exit())
        break;

      AsyncExecutor::invoke_async(
          [] {
            const auto result = vr_reset_rom();
            show_error_dialog_for_result(result);
          },
          ASYNC_KEY_RESET_ROM);
      break;

    case IDM_SETTINGS: {
      BetterEmulationLock lock;
      configdialog_show();
    } break;
    case IDM_ABOUT: {
      BetterEmulationLock lock;
      configdialog_about();
    } break;
    case IDM_COREDBG:
      CoreDbg::show();
      break;
    case IDM_SEEKER: {
      BetterEmulationLock lock;
      Seeker::show();
    } break;
    case IDM_RUNNER: {
      BetterEmulationLock lock;
      Runner::show();
    } break;
    case IDM_PIANO_ROLL:
      PianoRoll::show();
      break;
    case IDM_CHEATS: {
      BetterEmulationLock lock;
      Cheats::show();
    } break;
    case IDM_RAMSTART: {
      BetterEmulationLock lock;

      wchar_t ram_start[20] = {0};
      wsprintfW(ram_start, L"0x%p", static_cast<void *>(rdram));

      wchar_t proc_name[MAX_PATH] = {0};
      GetModuleFileName(NULL, proc_name, MAX_PATH);
      _wsplitpath(proc_name, 0, 0, proc_name, 0);

      wchar_t stroop_c[1024] = {0};
      wsprintfW(stroop_c,
                L"<Emulator name=\"Mupen 5.0 RR\" processName=\"%s\" "
                L"ramStart=\"%s\" endianness=\"little\" autoDetect=\"true\"/>",
                proc_name, ram_start);

      const auto str = std::format(
          L"The RAM start is {}.\r\nHow would you like to proceed?", ram_start);

      const auto result = FrontendService::show_multiple_choice_dialog(
          {L"Copy STROOP config line", L"Close"}, str.c_str(),
          L"Show RAM Start", FrontendService::DialogType::Information);

      if (result == 0) {
        copy_to_clipboard(g_main_hwnd, stroop_c);
      }

      break;
    }
    case IDM_STATS: {
      BetterEmulationLock lock;

      auto str = std::format(L"Total playtime: {}\r\nTotal rerecords: {}",
                             format_duration(g_config.total_frames / 30),
                             g_config.total_rerecords);

      MessageBoxW(g_main_hwnd, str.c_str(), L"Statistics", MB_ICONINFORMATION);
      break;
    }
    case IDM_CONSOLE:
      open_console();
      break;
    case IDM_LOAD_ROM: {
      BetterEmulationLock lock;

      const auto path = show_persistent_open_dialog(
          L"o_rom", g_main_hwnd,
          L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap");

      if (!path.empty()) {
        AsyncExecutor::invoke_async([path] {
          const auto result = vr_start_rom(path);
          show_error_dialog_for_result(result);
        });
      }
    } break;
    case IDM_EXIT:
      DestroyWindow(g_main_hwnd);
      break;
    case IDM_FULLSCREEN:
      toggle_fullscreen_mode();
      break;
    case IDM_REFRESH_ROMBROWSER:
      if (!emu_launched) {
        Rombrowser::build();
      }
      break;
    case IDM_SAVE_SLOT:
      ++g_vr_wait_before_input_poll;
      AsyncExecutor::invoke_async([=] {
        --g_vr_wait_before_input_poll;
        Savestates::do_slot(g_config.st_slot, Savestates::Job::Save);
      });
      break;
    case IDM_SAVE_STATE_AS: {
      BetterEmulationLock lock;

      auto path = show_persistent_save_dialog(L"s_savestate", hwnd,
                                              L"*.st;*.savestate");
      if (path.empty()) {
        break;
      }

      ++g_vr_wait_before_input_poll;
      AsyncExecutor::invoke_async([=] {
        --g_vr_wait_before_input_poll;
        Savestates::do_file(path, Savestates::Job::Save);
      });
    } break;
    case IDM_LOAD_SLOT:
      ++g_vr_wait_before_input_poll;
      AsyncExecutor::invoke_async([=] {
        --g_vr_wait_before_input_poll;
        Savestates::do_slot(g_config.st_slot, Savestates::Job::Load);
      });
      break;
    case IDM_LOAD_STATE_AS: {
      BetterEmulationLock lock;

      auto path = show_persistent_open_dialog(
          L"o_state", hwnd,
          L"*.st;*.savestate;*.st0;*.st1;*.st2;*.st3;*.st4;*.st5;*.st6;*.st7;*."
          L"st8;*.st9,*.st10");
      if (path.empty()) {
        break;
      }

      ++g_vr_wait_before_input_poll;
      AsyncExecutor::invoke_async([=] {
        --g_vr_wait_before_input_poll;
        Savestates::do_file(path, Savestates::Job::Load);
      });
    } break;
    case IDM_UNDO_LOAD_STATE: {
      ++g_vr_wait_before_input_poll;
      AsyncExecutor::invoke_async([=] {
        --g_vr_wait_before_input_poll;

        auto buf = Savestates::get_undo_savestate();

        if (buf.empty()) {
          Statusbar::post(L"No load to undo");
          return;
        }

        Savestates::do_memory(buf, Savestates::Job::Load,
                              [](const CoreResult result, auto) {
                                if (result == CoreResult::Ok) {
                                  Statusbar::post(L"Undid load");
                                  return;
                                }

                                if (result == CoreResult::ST_Cancelled) {
                                  return;
                                }

                                Statusbar::post(L"Failed to undo load");
                              });
      });
    } break;
    case IDM_START_MOVIE_RECORDING: {
      BetterEmulationLock lock;

      auto movie_dialog_result = MovieDialog::show(false);

      if (movie_dialog_result.path.empty()) {
        break;
      }

      auto vcr_result = VCR::start_record(
          movie_dialog_result.path, movie_dialog_result.start_flag,
          wstring_to_string(movie_dialog_result.author),
          wstring_to_string(movie_dialog_result.description));
      if (show_error_dialog_for_result(vcr_result)) {
        break;
      }

      g_config.last_movie_author = movie_dialog_result.author;

      Statusbar::post(L"Recording replay");
    } break;
    case IDM_START_MOVIE_PLAYBACK: {
      BetterEmulationLock lock;

      auto result = MovieDialog::show(true);

      if (result.path.empty()) {
        break;
      }

      VCR::replace_author_info(result.path, wstring_to_string(result.author),
                               wstring_to_string(result.description));

      g_config.pause_at_frame = result.pause_at;
      g_config.pause_at_last_frame = result.pause_at_last;

      AsyncExecutor::invoke_async([result] {
        auto vcr_result = VCR::start_playback(result.path);
        show_error_dialog_for_result(vcr_result);
      });
    } break;
    case IDM_STOP_MOVIE:
      VCR::stop_all();
      ClearButtons();
      break;
    case IDM_START_CAPTURE_PRESET:
    case IDM_START_CAPTURE:
      if (emu_launched) {
        BetterEmulationLock lock;

        auto path = show_persistent_save_dialog(L"s_capture", hwnd, L"*.avi");
        if (path.empty()) {
          break;
        }

        // bool vfw = MessageBox(mainHWND, "Use VFW for capturing?", "Capture",
        // MB_YESNO | MB_ICONQUESTION) == IDYES; auto container = vfw ?
        // EncodingManager::EncoderType::VFW :
        // EncodingManager::EncoderType::FFmpeg;
        bool ask_preset = LOWORD(wParam) == IDM_START_CAPTURE;

        // pass false to startCapture when "last preset" option was choosen
        if (EncodingManager::start_capture(
                path, static_cast<EncoderType>(g_config.encoder_type),
                ask_preset)) {
          Statusbar::post(L"Capture started...");
        }
      }

      break;
    case IDM_STOP_CAPTURE:
      EncodingManager::stop_capture();
      Statusbar::post(L"Capture stopped");
      break;
    case IDM_SCREENSHOT:
      CaptureScreen(get_screenshots_directory().string().data());
      break;
    case IDM_RESET_RECENT_ROMS:
      Recent::build(g_config.recent_rom_paths, ID_RECENTROMS_FIRST,
                    g_recent_roms_menu, true);
      break;
    case IDM_RESET_RECENT_MOVIES:
      Recent::build(g_config.recent_movie_paths, ID_RECENTMOVIES_FIRST,
                    g_recent_movies_menu, true);
      break;
    case IDM_RESET_RECENT_LUA:
      Recent::build(g_config.recent_lua_script_paths, ID_LUA_RECENT,
                    g_recent_lua_menu, true);
      break;
    case IDM_FREEZE_RECENT_ROMS:
      g_config.is_recent_rom_paths_frozen ^= true;
      break;
    case IDM_FREEZE_RECENT_MOVIES:
      g_config.is_recent_movie_paths_frozen ^= true;
      break;
    case IDM_FREEZE_RECENT_LUA:
      g_config.is_recent_scripts_frozen ^= true;
      break;
    case IDM_LOAD_LATEST_LUA:
      SendMessage(g_main_hwnd, WM_COMMAND, MAKEWPARAM(ID_LUA_RECENT, 0), 0);
      break;
    case IDM_LOAD_LATEST_ROM:
      SendMessage(g_main_hwnd, WM_COMMAND, MAKEWPARAM(ID_RECENTROMS_FIRST, 0),
                  0);
      break;
    case IDM_PLAY_LATEST_MOVIE:
      SendMessage(g_main_hwnd, WM_COMMAND, MAKEWPARAM(ID_RECENTMOVIES_FIRST, 0),
                  0);
      break;
    case IDM_STATUSBAR:
      g_config.is_statusbar_enabled ^= true;
      Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged,
                           (bool)g_config.is_statusbar_enabled);
      break;
    case IDC_DECREASE_MODIFIER:
      g_config.fps_modifier = clamp(g_config.fps_modifier - 25, 25, 1000);
      timer_init(g_config.fps_modifier, &ROM_HEADER);
      break;
    case IDC_INCREASE_MODIFIER:
      g_config.fps_modifier = clamp(g_config.fps_modifier + 25, 25, 1000);
      timer_init(g_config.fps_modifier, &ROM_HEADER);
      break;
    case IDC_RESET_MODIFIER:
      g_config.fps_modifier = 100;
      timer_init(g_config.fps_modifier, &ROM_HEADER);
      break;
    default:
      if (LOWORD(wParam) >= IDM_SELECT_1 && LOWORD(wParam) <= IDM_SELECT_10) {
        auto slot = LOWORD(wParam) - IDM_SELECT_1;
        g_config.st_slot = slot;
        Messenger::broadcast(Messenger::Message::SlotChanged,
                             static_cast<size_t>(g_config.st_slot));
      } else if (LOWORD(wParam) >= ID_SAVE_1 && LOWORD(wParam) <= ID_SAVE_10) {
        auto slot = LOWORD(wParam) - ID_SAVE_1;
        ++g_vr_wait_before_input_poll;
        AsyncExecutor::invoke_async([=] {
          --g_vr_wait_before_input_poll;
          Savestates::do_slot(slot, Savestates::Job::Save);
        });
      } else if (LOWORD(wParam) >= ID_LOAD_1 && LOWORD(wParam) <= ID_LOAD_10) {
        auto slot = LOWORD(wParam) - ID_LOAD_1;
        ++g_vr_wait_before_input_poll;
        AsyncExecutor::invoke_async([=] {
          --g_vr_wait_before_input_poll;
          Savestates::do_slot(slot, Savestates::Job::Load);
        });
      } else if (LOWORD(wParam) >= ID_RECENTROMS_FIRST &&
                 LOWORD(wParam) <
                     (ID_RECENTROMS_FIRST + g_config.recent_rom_paths.size())) {
        auto path = Recent::element_at(g_config.recent_rom_paths,
                                       ID_RECENTROMS_FIRST, LOWORD(wParam));
        if (path.empty())
          break;

        AsyncExecutor::invoke_async(
            [path] {
              const auto result = vr_start_rom(path);
              show_error_dialog_for_result(result);
            },
            ASYNC_KEY_START_ROM);
      } else if (LOWORD(wParam) >= ID_RECENTMOVIES_FIRST &&
                 LOWORD(wParam) < (ID_RECENTMOVIES_FIRST +
                                   g_config.recent_movie_paths.size())) {
        auto path = Recent::element_at(g_config.recent_movie_paths,
                                       ID_RECENTMOVIES_FIRST, LOWORD(wParam));
        if (path.empty())
          break;

        g_config.vcr_readonly = true;
        Messenger::broadcast(Messenger::Message::ReadonlyChanged,
                             (bool)g_config.vcr_readonly);
        AsyncExecutor::invoke_async(
            [path] {
              auto result = VCR::start_playback(path);
              show_error_dialog_for_result(result);
            },
            ASYNC_KEY_PLAY_MOVIE);
      } else if (LOWORD(wParam) >= ID_LUA_RECENT &&
                 LOWORD(wParam) < (ID_LUA_RECENT +
                                   g_config.recent_lua_script_paths.size())) {
        auto path = Recent::element_at(g_config.recent_lua_script_paths,
                                       ID_LUA_RECENT, LOWORD(wParam));
        if (path.empty())
          break;

        lua_create_and_run(path);
      }
      break;
    }
  } break;
  default:
    return DefWindowProc(hwnd, Message, wParam, lParam);
  }

  return TRUE;
}

// kaboom
LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS *ExceptionInfo) {
  CrashHelper::log_crash(ExceptionInfo);

  if (g_config.silent_mode) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  bool is_continuable = !(ExceptionInfo->ExceptionRecord->ExceptionFlags &
                          EXCEPTION_NONCONTINUABLE);

  int result = 0;

  if (is_continuable) {
    TaskDialog(g_main_hwnd, g_app_instance, L"Error", L"An error has occured",
               L"A crash log has been automatically generated. You can choose "
               L"to continue program execution.",
               TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);
  } else {
    TaskDialog(g_main_hwnd, g_app_instance, L"Error", L"An error has occured",
               L"A crash log has been automatically generated. The program "
               L"will now exit.",
               TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);
  }

  if (result == IDCLOSE) {
    return EXCEPTION_EXECUTE_HANDLER;
  }
  return EXCEPTION_CONTINUE_EXECUTION;
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
#ifdef _DEBUG
  open_console();
#endif

  Loggers::init();

  g_view_logger->info("WinMain");

  g_ui_thread_id = GetCurrentThreadId();

  Gdiplus::GdiplusStartupInput startup_input;
  GdiplusStartup(&gdi_plus_token, &startup_input, NULL);

  Messenger::init();
  AsyncExecutor::init();
  CoreDbg::init();

  g_app_path = get_app_full_path();

  wchar_t cwd[MAX_PATH] = {0};
  GetCurrentDirectory(std::size(cwd), cwd);

  // CWD is sometimes messed up when running from CLI, so we fix it here
  // Needs to be fixed prior to loading config, since that uses a relative path
  SetCurrentDirectory(g_app_path.c_str());

  g_app_instance = hInstance;

  // ensure folders exist!
  CreateDirectory((g_app_path / L"save").c_str(), NULL);
  CreateDirectory((g_app_path / L"screenshots").c_str(), NULL);
  CreateDirectory((g_app_path / L"plugin").c_str(), NULL);
  CreateDirectory((g_app_path / L"backups").c_str(), NULL);

  init_config();
  load_config();
  lua_init();

  if (g_config.keep_default_working_directory) {
    SetCurrentDirectory(cwd);
  }

  // Log cwd again for fun
  GetCurrentDirectory(sizeof(cwd), cwd);
  g_view_logger->info(L"cwd: {}", cwd);

  WNDCLASSEX wc = {0};
  MSG msg;

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_M64ICONBIG));
  wc.hIconSm = LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_M64ICONSMALL));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = WND_CLASS;
  wc.lpfnWndProc = WndProc;
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);

  RegisterClassEx(&wc);
  MGECompositor::init();

  g_view_logger->info("[View] Restoring window @ ({}|{}) {}x{}...",
                      g_config.window_x, g_config.window_y,
                      g_config.window_width, g_config.window_height);

  g_main_hwnd = CreateWindowEx(
      0, WND_CLASS, MUPEN_VERSION, WS_OVERLAPPEDWINDOW | WS_EX_COMPOSITED,
      g_config.window_x, g_config.window_y, g_config.window_width,
      g_config.window_height, NULL, NULL, hInstance, NULL);

  ShowWindow(g_main_hwnd, nCmdShow);
  SetWindowLong(g_main_hwnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES);

  g_recent_roms_menu = GetSubMenu(GetSubMenu(g_main_menu, 0), 5);
  g_recent_movies_menu = GetSubMenu(GetSubMenu(g_main_menu, 3), 5);
  g_recent_lua_menu = GetSubMenu(GetSubMenu(g_main_menu, 6), 2);
#ifndef _DEBUG
#endif

  RECT rect{};
  GetClientRect(g_main_hwnd, &rect);

  Messenger::subscribe(Messenger::Message::EmuLaunchedChanged,
                       on_emu_launched_changed);
  Messenger::subscribe(Messenger::Message::EmuStopping, on_emu_stopping);
  Messenger::subscribe(Messenger::Message::EmuPausedChanged,
                       on_emu_paused_changed);
  Messenger::subscribe(Messenger::Message::CapturingChanged,
                       on_capturing_changed);
  Messenger::subscribe(Messenger::Message::MovieLoopChanged,
                       on_movie_loop_changed);
  Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
  Messenger::subscribe(Messenger::Message::ScriptStarted, on_script_started);
  Messenger::subscribe(Messenger::Message::SpeedModifierChanged,
                       on_speed_modifier_changed);
  Messenger::subscribe(Messenger::Message::LagLimitExceeded,
                       on_vis_since_input_poll_exceeded);
  Messenger::subscribe(Messenger::Message::FullscreenChanged,
                       on_fullscreen_changed);
  Messenger::subscribe(Messenger::Message::ConfigLoaded, on_config_loaded);
  Messenger::subscribe(Messenger::Message::SeekCompleted, on_seek_completed);
  Messenger::subscribe(Messenger::Message::WarpModifyStatusChanged,
                       on_warp_modify_status_changed);
  Messenger::subscribe(Messenger::Message::FastForwardNeedsUpdate,
                       update_core_fast_forward);
  Messenger::subscribe(Messenger::Message::SeekStatusChanged,
                       [](std::any) { update_core_fast_forward(nullptr); });
  Messenger::subscribe(Messenger::Message::EmuStartingChanged,
                       [](std::any data) {
                         g_emu_starting = std::any_cast<bool>(data);
                         update_titlebar();
                       });

  // Rombrowser needs to be initialized *after* other components, since it
  // depends on their state smh bru
  Statusbar::init();
  Rombrowser::init();
  VCR::init();
  EncodingManager::init();
  Cli::init();
  Seeker::init();
  Savestates::init();
  setup_dummy_info();
  update_core_fast_forward(nullptr);

  Recent::build(g_config.recent_rom_paths, ID_RECENTROMS_FIRST,
                g_recent_roms_menu);
  Recent::build(g_config.recent_movie_paths, ID_RECENTMOVIES_FIRST,
                g_recent_movies_menu);
  Recent::build(g_config.recent_lua_script_paths, ID_LUA_RECENT,
                g_recent_lua_menu);

  Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged,
                       (bool)g_config.is_statusbar_enabled);
  Messenger::broadcast(Messenger::Message::MovieLoopChanged,
                       (bool)g_config.is_movie_loop_enabled);
  Messenger::broadcast(Messenger::Message::ReadonlyChanged,
                       (bool)g_config.vcr_readonly);
  Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, false);
  Messenger::broadcast(Messenger::Message::CoreExecutingChanged, false);
  Messenger::broadcast(Messenger::Message::CapturingChanged, false);
  Messenger::broadcast(Messenger::Message::SizeChanged, rect);
  Messenger::broadcast(Messenger::Message::AppReady, nullptr);
  Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);

  // warning, this is ignored when debugger is attached (like visual studio)
  SetUnhandledExceptionFilter(ExceptionReleaseTarget);

  // raise noncontinuable exception (impossible to recover from it)
  // RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL,
  // NULL);
  //
  // raise continuable exception
  // RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);

  // We need to set the core updateScreen flag at 60 FPS.
  // WM_TIMER isn't stable enough and the other multimedia or callback timers
  // are too annoying
  std::thread([] {
    while (true) {
      screen_invalidated = true;
      timeBeginPeriod(1);
      Sleep(1000 / 60);
      timeEndPeriod(1);
    }
  }).detach();

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}
