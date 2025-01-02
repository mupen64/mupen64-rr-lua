#pragma once
#include "lib/microlru.h"
#include "presenters/Presenter.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <Windows.h>
#include <assert.h>
#include <cstdint>
#include <d2d1.h>
#include <dcomp.h>
#include <dwrite.h>
#include <filesystem>
#include <functional>
#include <gdiplus.h>
#include <locale>
#include <map>
#include <queue>
#include <stack>
#include <string>

#include <core/r4300/Plugin.hpp>

typedef struct s_window_procedure_params {
  HWND wnd;
  UINT msg;
  WPARAM w_param;
  LPARAM l_param;
} t_window_procedure_params;

/**
 * \brief Initializes the lua subsystem
 */
void lua_init();

/**
 * \brief Creates a lua window and runs the specified script
 * \param path The script's path
 */
void lua_create_and_run(const std::wstring &path);

/**
 * \brief Creates a lua window
 * \return The window's handle
 */
HWND lua_create();

/**
 * \brief Stops all lua scripts and closes their windows
 */
void close_all_scripts();

/**
 * \brief Stops all lua scripts
 */
void stop_all_scripts();

/**
 * \brief Invokes a lua callback on all instances
 * \param function The native function
 * \param key The function's registration key
 */
void invoke_callbacks_with_key_on_all_instances(
    const std::function<int(lua_State *)> &function, const char *key);

static const auto REG_LUACLASS = "C";
static const auto REG_ATUPDATESCREEN = "S";
static const auto REG_ATDRAWD2D = "SD2D";
static const auto REG_ATVI = "V";
static const auto REG_ATINPUT = "I";
static const auto REG_ATSTOP = "T";
static const auto REG_SYNCBREAK = "B";
static const auto REG_READBREAK = "R";
static const auto REG_WRITEBREAK = "W";
static const auto REG_WINDOWMESSAGE = "M";
static const auto REG_ATINTERVAL = "N";
static const auto REG_ATPLAYMOVIE = "PM";
static const auto REG_ATSTOPMOVIE = "SM";
static const auto REG_ATLOADSTATE = "LS";
static const auto REG_ATSAVESTATE = "SS";
static const auto REG_ATRESET = "RE";
static const auto REG_ATSEEKCOMPLETED = "SC";
static const auto REG_ATWARPMODIFYSTATUSCHANGED = "WS";

static uint32_t lua_gdi_color_mask = RGB(255, 0, 255);
static HBRUSH alpha_mask_brush = CreateSolidBrush(lua_gdi_color_mask);

class LuaEnvironment {
public:
  /**
   * \brief Creates a lua environment, adding it to the global map and running
   * it if the operation succeeds \param path The script path \param wnd The
   * associated window \return An error string, or an empty string if the
   * operation succeeded
   */
  static std::string create(const std::filesystem::path &path, HWND wnd);

  /**
   * \brief Stops, destroys and removes a lua environment from the environment
   * map \param lua_environment The lua environment to destroy
   */
  static void destroy(LuaEnvironment *lua_environment);

  /**
   * \brief Prints text to a lua environment dialog's console
   * \param hwnd Handle to a lua environment dialog of IDD_LUAWINDOW
   * \param text The text to display
   */
  static void print_con(HWND hwnd, const std::wstring &text);

  // The path to the current lua script
  std::filesystem::path path;

  // The current presenter, or null
  Presenter *presenter;

  // The Direct2D overlay control handle
  HWND d2d_overlay_hwnd;

  // The GDI/GDI+ overlay control handle
  HWND gdi_overlay_hwnd;

  // The DC for GDI/GDI+ drawings
  // This DC is special, since commands can be issued to it anytime and it's
  // never cleared
  HDC gdi_back_dc = nullptr;

  // The bitmap for GDI/GDI+ drawings
  HBITMAP gdi_bmp;

  // Dimensions of the drawing surfaces
  D2D1_SIZE_U dc_size;

  // The DirectWrite factory, whose lifetime is the renderer's
  IDWriteFactory *dw_factory = nullptr;

  // The cache for DirectWrite text layouts
  MicroLRU::Cache<uint64_t, IDWriteTextLayout *> dw_text_layouts;

  // The stack of render targets. The top is used for D2D calls.
  std::stack<ID2D1RenderTarget *> d2d_render_target_stack;

  // Pool of GDI+ images
  std::unordered_map<size_t, Gdiplus::Bitmap *> image_pool;

  // Amount of generated images, just used to generate uids for image pool
  size_t image_pool_index;

  HDC loadscreen_dc;
  HBITMAP loadscreen_bmp;

  HBRUSH brush;
  HPEN pen;
  HFONT font;
  COLORREF col, bkcol;
  int bkmode;

  LuaEnvironment() = default;

  /**
   * \brief Destroys and stops the environment
   */
  ~LuaEnvironment();

  void create_renderer();
  void destroy_renderer();

  void create_loadscreen();
  void destroy_loadscreen();

  // calls all functions that lua script has defined as callbacks, reads them
  // from registry returns true at fail
  bool
  invoke_callbacks_with_key(const std::function<int(lua_State *)> &function,
                            const char *key);

  // Invalidates the composition layer
  void invalidate_visuals();

  // Repaints the composition layer
  void repaint_visuals();

  HWND hwnd;
  lua_State *L;

  /**
   * \brief Prints text to the environment's console
   * \param text The text to print
   */
  void print(const std::wstring &text) const { print_con(hwnd, text); }

private:
  void register_functions();
};

extern uint64_t inputCount;

/**
 * \brief The controller data at time of the last input poll
 */
extern BUTTONS last_controller_data[4];

/**
 * \brief The modified control data to be pushed the next frame
 */
extern BUTTONS new_controller_data[4];

/**
 * \brief Whether the <c>new_controller_data</c> of a controller should be
 * pushed the next frame
 */
extern bool overwrite_controller_data[4];

extern std::map<HWND, LuaEnvironment *> g_hwnd_lua_map;

/**
 * \brief Gets the Lua environment associated with a lua state.
 */
LuaEnvironment *get_lua_class(lua_State *lua_state);

/**
 * \brief Registers a function with a key to a lua state
 * \param L The lua state
 * \param key The function's key
 */
int RegisterFunction(lua_State *L, const char *key);

/**
 * \brief Unregisters a function with a key to a lua state
 * \param L The lua state
 * \param key The function's key
 */
void UnregisterFunction(lua_State *L, const char *key);
