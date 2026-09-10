--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

---@meta

emu = {}
memory = {}
debugger = {}
wgui = {}
painter = {}
input = {}
joypad = {}
movie = {}
savestate = {}
iohelper = {}
avi = {}
hotkey = {}
action = {}
clipboard = {}

Mupen = {
    _VERSION = '1.5.0-3',
    _URL = 'https://github.com/mupen64/mupen64-rr-lua',
    _DESCRIPTION = 'Mupen64 Lua Scripting API',
    _LICENSE = 'GPL-2',

    ---@enum Keycode
    ---SDL keycodes used by hotkeys and keyboard events.
    keycode = {
        SDLK_UNKNOWN = 0x00000000,
        SDLK_RETURN = 0x0000000D,
        SDLK_ESCAPE = 0x0000001B,
        SDLK_BACKSPACE = 0x00000008,
        SDLK_TAB = 0x00000009,
        SDLK_SPACE = 0x00000020,
        SDLK_DELETE = 0x0000007F,

        SDLK_0 = 0x00000030,
        SDLK_1 = 0x00000031,
        SDLK_2 = 0x00000032,
        SDLK_3 = 0x00000033,
        SDLK_4 = 0x00000034,
        SDLK_5 = 0x00000035,
        SDLK_6 = 0x00000036,
        SDLK_7 = 0x00000037,
        SDLK_8 = 0x00000038,
        SDLK_9 = 0x00000039,
        SDLK_A = 0x00000061,
        SDLK_B = 0x00000062,
        SDLK_C = 0x00000063,
        SDLK_D = 0x00000064,
        SDLK_E = 0x00000065,
        SDLK_F = 0x00000066,
        SDLK_G = 0x00000067,
        SDLK_H = 0x00000068,
        SDLK_I = 0x00000069,
        SDLK_J = 0x0000006A,
        SDLK_K = 0x0000006B,
        SDLK_L = 0x0000006C,
        SDLK_M = 0x0000006D,
        SDLK_N = 0x0000006E,
        SDLK_O = 0x0000006F,
        SDLK_P = 0x00000070,
        SDLK_Q = 0x00000071,
        SDLK_R = 0x00000072,
        SDLK_S = 0x00000073,
        SDLK_T = 0x00000074,
        SDLK_U = 0x00000075,
        SDLK_V = 0x00000076,
        SDLK_W = 0x00000077,
        SDLK_X = 0x00000078,
        SDLK_Y = 0x00000079,
        SDLK_Z = 0x0000007A,

        SDLK_PLUS = 0x0000002B,
        SDLK_COMMA = 0x0000002C,
        SDLK_MINUS = 0x0000002D,
        SDLK_PERIOD = 0x0000002E,
        SDLK_SLASH = 0x0000002F,
        SDLK_SEMICOLON = 0x0000003B,
        SDLK_EQUALS = 0x0000003D,
        SDLK_LEFTBRACKET = 0x0000005B,
        SDLK_BACKSLASH = 0x0000005C,
        SDLK_RIGHTBRACKET = 0x0000005D,
        SDLK_GRAVE = 0x00000060,
        SDLK_APOSTROPHE = 0x00000027,

        SDLK_CAPSLOCK = 0x40000039,
        SDLK_F1 = 0x4000003A,
        SDLK_F2 = 0x4000003B,
        SDLK_F3 = 0x4000003C,
        SDLK_F4 = 0x4000003D,
        SDLK_F5 = 0x4000003E,
        SDLK_F6 = 0x4000003F,
        SDLK_F7 = 0x40000040,
        SDLK_F8 = 0x40000041,
        SDLK_F9 = 0x40000042,
        SDLK_F10 = 0x40000043,
        SDLK_F11 = 0x40000044,
        SDLK_F12 = 0x40000045,
        SDLK_F13 = 0x40000068,
        SDLK_F14 = 0x40000069,
        SDLK_F15 = 0x4000006A,
        SDLK_F16 = 0x4000006B,
        SDLK_F17 = 0x4000006C,
        SDLK_F18 = 0x4000006D,
        SDLK_F19 = 0x4000006E,
        SDLK_F20 = 0x4000006F,
        SDLK_F21 = 0x40000070,
        SDLK_F22 = 0x40000071,
        SDLK_F23 = 0x40000072,
        SDLK_F24 = 0x40000073,
        SDLK_PRINTSCREEN = 0x40000046,
        SDLK_SCROLLLOCK = 0x40000047,
        SDLK_PAUSE = 0x40000048,
        SDLK_INSERT = 0x40000049,
        SDLK_HOME = 0x4000004A,
        SDLK_PAGEUP = 0x4000004B,
        SDLK_END = 0x4000004D,
        SDLK_PAGEDOWN = 0x4000004E,
        SDLK_RIGHT = 0x4000004F,
        SDLK_LEFT = 0x40000050,
        SDLK_DOWN = 0x40000051,
        SDLK_UP = 0x40000052,

        SDLK_NUMLOCKCLEAR = 0x40000053,
        SDLK_KP_DIVIDE = 0x40000054,
        SDLK_KP_MULTIPLY = 0x40000055,
        SDLK_KP_MINUS = 0x40000056,
        SDLK_KP_PLUS = 0x40000057,
        SDLK_KP_ENTER = 0x40000058,
        SDLK_KP_1 = 0x40000059,
        SDLK_KP_2 = 0x4000005A,
        SDLK_KP_3 = 0x4000005B,
        SDLK_KP_4 = 0x4000005C,
        SDLK_KP_5 = 0x4000005D,
        SDLK_KP_6 = 0x4000005E,
        SDLK_KP_7 = 0x4000005F,
        SDLK_KP_8 = 0x40000060,
        SDLK_KP_9 = 0x40000061,
        SDLK_KP_0 = 0x40000062,
        SDLK_KP_PERIOD = 0x40000063,
        SDLK_APPLICATION = 0x40000065,
        SDLK_KP_EQUALS = 0x40000067,

        SDLK_LCTRL = 0x400000E0,
        SDLK_LSHIFT = 0x400000E1,
        SDLK_LALT = 0x400000E2,
        SDLK_LGUI = 0x400000E3,
        SDLK_RCTRL = 0x400000E4,
        SDLK_RSHIFT = 0x400000E5,
        SDLK_RALT = 0x400000E6,
        SDLK_RGUI = 0x400000E7,
        SDLK_SLEEP = 0x40000102,
        SDLK_HELP = 0x40000075,
        SDLK_MENU = 0x40000076,
        SDLK_SELECT = 0x40000077,
        SDLK_EXECUTE = 0x40000074,
        SDLK_CLEAR = 0x4000009C,
        SDLK_PRIOR = 0x4000009D,
        SDLK_SEPARATOR = 0x4000009F,
        SDLK_MUTE = 0x4000007F,
        SDLK_VOLUMEUP = 0x40000080,
        SDLK_VOLUMEDOWN = 0x40000081,

        SDLK_EXCLAIM = 0x00000021,
        SDLK_DBLAPOSTROPHE = 0x00000022,
        SDLK_HASH = 0x00000023,
        SDLK_DOLLAR = 0x00000024,
        SDLK_PERCENT = 0x00000025,
        SDLK_AMPERSAND = 0x00000026,
        SDLK_LEFTPAREN = 0x00000028,
        SDLK_RIGHTPAREN = 0x00000029,
        SDLK_ASTERISK = 0x0000002A,
        SDLK_COLON = 0x0000003A,
        SDLK_LESS = 0x0000003C,
        SDLK_GREATER = 0x0000003E,
        SDLK_QUESTION = 0x0000003F,
        SDLK_AT = 0x00000040,
        SDLK_CARET = 0x0000005E,
        SDLK_UNDERSCORE = 0x0000005F,
        SDLK_LEFTBRACE = 0x0000007B,
        SDLK_PIPE = 0x0000007C,
        SDLK_RIGHTBRACE = 0x0000007D,
        SDLK_TILDE = 0x0000007E,
        SDLK_PLUSMINUS = 0x000000B1,
        SDLK_POWER = 0x40000066,
        SDLK_STOP = 0x40000078,
        SDLK_AGAIN = 0x40000079,
        SDLK_UNDO = 0x4000007A,
        SDLK_CUT = 0x4000007B,
        SDLK_COPY = 0x4000007C,
        SDLK_PASTE = 0x4000007D,
        SDLK_FIND = 0x4000007E,
        SDLK_KP_COMMA = 0x40000085,
        SDLK_KP_EQUALSAS400 = 0x40000086,
        SDLK_ALTERASE = 0x40000099,
        SDLK_SYSREQ = 0x4000009A,
        SDLK_CANCEL = 0x4000009B,
        SDLK_RETURN2 = 0x4000009E,
        SDLK_OUT = 0x400000A0,
        SDLK_OPER = 0x400000A1,
        SDLK_CLEARAGAIN = 0x400000A2,
        SDLK_CRSEL = 0x400000A3,
        SDLK_EXSEL = 0x400000A4,
        SDLK_KP_00 = 0x400000B0,
        SDLK_KP_000 = 0x400000B1,
        SDLK_THOUSANDSSEPARATOR = 0x400000B2,
        SDLK_DECIMALSEPARATOR = 0x400000B3,
        SDLK_CURRENCYUNIT = 0x400000B4,
        SDLK_CURRENCYSUBUNIT = 0x400000B5,
        SDLK_KP_LEFTPAREN = 0x400000B6,
        SDLK_KP_RIGHTPAREN = 0x400000B7,
        SDLK_KP_LEFTBRACE = 0x400000B8,
        SDLK_KP_RIGHTBRACE = 0x400000B9,
        SDLK_KP_TAB = 0x400000BA,
        SDLK_KP_BACKSPACE = 0x400000BB,
        SDLK_KP_A = 0x400000BC,
        SDLK_KP_B = 0x400000BD,
        SDLK_KP_C = 0x400000BE,
        SDLK_KP_D = 0x400000BF,
        SDLK_KP_E = 0x400000C0,
        SDLK_KP_F = 0x400000C1,
        SDLK_KP_XOR = 0x400000C2,
        SDLK_KP_POWER = 0x400000C3,
        SDLK_KP_PERCENT = 0x400000C4,
        SDLK_KP_LESS = 0x400000C5,
        SDLK_KP_GREATER = 0x400000C6,
        SDLK_KP_AMPERSAND = 0x400000C7,
        SDLK_KP_DBLAMPERSAND = 0x400000C8,
        SDLK_KP_VERTICALBAR = 0x400000C9,
        SDLK_KP_DBLVERTICALBAR = 0x400000CA,
        SDLK_KP_COLON = 0x400000CB,
        SDLK_KP_HASH = 0x400000CC,
        SDLK_KP_SPACE = 0x400000CD,
        SDLK_KP_AT = 0x400000CE,
        SDLK_KP_EXCLAM = 0x400000CF,
        SDLK_KP_MEMSTORE = 0x400000D0,
        SDLK_KP_MEMRECALL = 0x400000D1,
        SDLK_KP_MEMCLEAR = 0x400000D2,
        SDLK_KP_MEMADD = 0x400000D3,
        SDLK_KP_MEMSUBTRACT = 0x400000D4,
        SDLK_KP_MEMMULTIPLY = 0x400000D5,
        SDLK_KP_MEMDIVIDE = 0x400000D6,
        SDLK_KP_PLUSMINUS = 0x400000D7,
        SDLK_KP_CLEAR = 0x400000D8,
        SDLK_KP_CLEARENTRY = 0x400000D9,
        SDLK_KP_BINARY = 0x400000DA,
        SDLK_KP_OCTAL = 0x400000DB,
        SDLK_KP_DECIMAL = 0x400000DC,
        SDLK_KP_HEXADECIMAL = 0x400000DD,
        SDLK_MODE = 0x40000101,
        SDLK_WAKE = 0x40000103,
        SDLK_CHANNEL_INCREMENT = 0x40000104,
        SDLK_CHANNEL_DECREMENT = 0x40000105,
        SDLK_MEDIA_PLAY = 0x40000106,
        SDLK_MEDIA_PAUSE = 0x40000107,
        SDLK_MEDIA_RECORD = 0x40000108,
        SDLK_MEDIA_FAST_FORWARD = 0x40000109,
        SDLK_MEDIA_REWIND = 0x4000010A,
        SDLK_MEDIA_NEXT_TRACK = 0x4000010B,
        SDLK_MEDIA_PREVIOUS_TRACK = 0x4000010C,
        SDLK_MEDIA_STOP = 0x4000010D,
        SDLK_MEDIA_EJECT = 0x4000010E,
        SDLK_MEDIA_PLAY_PAUSE = 0x4000010F,
        SDLK_MEDIA_SELECT = 0x40000110,
        SDLK_AC_NEW = 0x40000111,
        SDLK_AC_OPEN = 0x40000112,
        SDLK_AC_CLOSE = 0x40000113,
        SDLK_AC_EXIT = 0x40000114,
        SDLK_AC_SAVE = 0x40000115,
        SDLK_AC_PRINT = 0x40000116,
        SDLK_AC_PROPERTIES = 0x40000117,
        SDLK_AC_SEARCH = 0x40000118,
        SDLK_AC_HOME = 0x40000119,
        SDLK_AC_BACK = 0x4000011A,
        SDLK_AC_FORWARD = 0x4000011B,
        SDLK_AC_STOP = 0x4000011C,
        SDLK_AC_REFRESH = 0x4000011D,
        SDLK_AC_BOOKMARKS = 0x4000011E,
        SDLK_SOFTLEFT = 0x4000011F,
        SDLK_SOFTRIGHT = 0x40000120,
        SDLK_CALL = 0x40000121,
        SDLK_ENDCALL = 0x40000122,
        SDLK_LEFT_TAB = 0x20000001,
        SDLK_LEVEL5_SHIFT = 0x20000002,
        SDLK_MULTI_KEY_COMPOSE = 0x20000003,
        SDLK_LMETA = 0x20000004,
        SDLK_RMETA = 0x20000005,
        SDLK_LHYPER = 0x20000006,
        SDLK_RHYPER = 0x20000007,
    },

    ---@enum MouseButtonFlags
    ---SDL mouse-button bitmasks.
    mousebutton = {
        SDL_BUTTON_LMASK = 0x01,
        SDL_BUTTON_MMASK = 0x02,
        SDL_BUTTON_RMASK = 0x04,
        SDL_BUTTON_X1MASK = 0x08,
        SDL_BUTTON_X2MASK = 0x10,
    },

    ---@enum Result
    ---An enum containing results that can be returned by the core.
    result = {
        -- Generic
        -- ==========================================

        -- The operation completed successfully
        res_ok = 0,

        -- The operation was cancelled by the user
        res_cancelled = 1,

        -- VCR
        -- ==========================================

        -- The provided data has an invalid format
        vcr_invalid_format = 2,

        -- The provided file is inaccessible or does not exist
        vcr_bad_file = 3,

        -- The cheat data couldn't be written to disk
        vcr_cheat_write_failed = 4,

        -- The controller configuration is invalid
        vcr_invalid_controllers = 5,

        -- The movie's savestate is missing or invalid
        vcr_invalid_savestate = 6,

        -- The resulting frame is outside the bounds of the movie
        vcr_invalid_frame = 7,

        -- There is no rom which matches this movie
        vcr_no_matching_rom = 8,

        -- The VCR engine is idle, but must be active to complete this operation
        vcr_idle = 9,

        -- The provided freeze buffer is not from the currently active movie
        vcr_not_from_this_movie = 10,

        -- The movie's version is invalid
        vcr_invalid_version = 11,

        -- The movie's extended version is invalid
        vcr_invalid_extended_version = 12,

        -- The operation requires a playback or recording task
        vcr_needs_playback_or_recording = 13,

        -- The operation requires a playback task
        vcr_needs_playback = 14,

        -- The provided start type is invalid
        vcr_invalid_start_type = 15,

        -- Another warp modify operation is already running
        vcr_warp_modify_already_running = 16,

        -- Warp modifications can only be performed during recording
        vcr_warp_modify_needs_recording_task = 17,

        -- The provided input buffer is empty
        vcr_warp_modify_empty_input_buffer = 18,

        -- The seek operation could not be initiated due to a savestate not being loaded successfully
        vcr_seek_savestate_load_failed = 19,

        -- The seek operation can't be initiated because the seek savestate interval is 0
        vcr_seek_savestate_interval_zero = 20,

        -- The seek string is malformed
        vcr_seek_string_malformed = 21,

        -- VR
        -- ==========================================

        -- Couldn't find a rom matching the provided movie
        vr_no_matching_rom = 22,

        -- An error occured during plugin loading
        vr_plugin_error = 23,

        -- The ROM or alternative rom source is invalid
        vr_rom_invalid = 24,

        -- The emulator isn't running yet
        vr_not_running = 25,

        -- Failed to open core streams
        vr_file_open_failed = 26,

        -- Savestates
        -- ==========================================

        -- The core isn't launched
        st_core_not_launched = 27,

        -- The savestate file wasn't found
        st_not_found = 28,

        -- The savestate couldn't be written to disk
        st_file_write_error = 29,

        -- Couldn't decompress the savestate
        st_decompression_error = 30,

        -- The event queue was too long
        st_event_queue_too_long = 31,

        -- The CPU registers contained invalid values
        st_invalid_registers = 32,

        -- Plugins
        -- ==========================================

        -- The plugin library couldn't be loaded
        pl_load_library_failed = 33,

        -- The plugin doesn't export a GetDllInfo function
        pl_no_get_dll_info = 34,

        -- Init
        -- ==========================================

        -- The core params are missing a critical component.
        in_missing_component = 35,
    },



    ---The speed mode of the core.
    ---@enum CoreSpeedMode
    CoreSpeedMode = {
        -- Normal speed mode. The speed cap is affected by the FPS modifier.
        Normal = 0,

        -- Fast forward speed mode. The speed cap is not affected by the FPS modifier.
        FastForward = 1,

        -- Ultra fast forward speed mode. The speed cap is not affected by the FPS modifier.
        -- Achieves maximum performance by unconditionally skipping invalidation, RSP, and potentially other miscellaneous
        -- steps.
        -- May affect video or audio fidelity.
        UltraFastForward = 2,
    }
}

---The `lua_tostring` c function converts numbers to strings, so numbers are
---acceptable to pass into some functions that use that function.
---@alias tostringusable string|number

---@class KeyEventArgs
---@field keycode Keycode? The SDL keycode, if the event is a key event.
---@field ctrl boolean Whether the Ctrl key is held down.
---@field alt boolean Whether the Alt key is held down.
---@field shift boolean Whether the Shift key is held down.
---@field meta boolean Whether the Meta key is held down.
---@field pressed boolean? Whether the key was pressed or released, if the event is a key event.
---@field text string? The typed character, if the event is a char event and the key corresponds to a character.
---@field repeat boolean Whether the event is a repeat event (i.e. the key is being held down and this event is firing multiple times).

---@alias MouseButton 0|1|2

---@class MouseEventArgs
---@field x integer The x-coordinate of the mouse relative to the main window.
---@field y integer The y-coordinate of the mouse relative to the main window.
---@field ctrl boolean Whether the Ctrl key is held down.
---@field alt boolean Whether the Alt key is held down.
---@field shift boolean Whether the Shift key is held down.
---@field meta boolean Whether the Meta key is held down.
---@field x_wheel integer? The horizontal scroll delta. If `nil`, the event is not related to horizontal scrolling. A positive value means scroll right, negative means scroll left. The magnitude is not specified.
---@field y_wheel integer? The vertical scroll delta. If `nil`, the event is not related to vertical scrolling. A positive value means scroll down, negative means scroll up. The magnitude is not specified.
---@field button MouseButton? The mouse button that was pressed or released. If `nil`, the event is not related to a mouse button.
---@field pressed boolean? Whether the mouse button was pressed or released. Only present if `button ~= nil`.
---@field double_click boolean? Whether the event is a double-click event. Only present if `button ~= nil`.

---@class CPUState
---@field opcode integer
---@field address integer

-- Global Functions
--#region

---Prints a value to the lua console.
---
---The data is formatted using the [inspect.lua library](https://github.com/kikito/inspect.lua).
---@param data any The data to print to the console.
---@return nil
function print(data) end

---Converts a value to a string using the [inspect.lua library](https://github.com/kikito/inspect.lua).
---@param value any
---@return string
function tostringex(value) end

---Stops script execution.
---@return nil
function stop() end

---Queues up the Mupen64 process to be stopped.
---
---**Script execution may continue after this function is called; make sure this behaviour is handled correctly.**
---@param code?  boolean|integer
---@param close? boolean
function os.exit(code, close) end

--#endregion


-- emu functions
--#region

---Displays the text `message` in the console. Similar to `print`, but only accepts strings or numbers.
---Also, `emu.console` does not insert a newline character.
---Because of this, `print` should be used instead.
---@deprecated Use `print` instead.
---@param message tostringusable The string to print to the console.
---@return nil
function emu.console(message) end

---Displays the text `message` in the status bar on the bottom while replacing any other text.
---The message will only display until the next frame.
---@param message tostringusable The string to display on the status bar.
---@return nil
function emu.statusbar(message) end

---Calls the function `f` every VI frame.
---For example, in Super Mario 64, the function will be called twice when you advance by one frame, whereas it will be called once in Ocarina of Time.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called every VI frame.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atvi(f, unregister) end

---Similar to `emu.atvi`, but for `wgui` drawing commands.
---Only drawing functions from the `wgui` namespace work here; painter commands do not.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called after every VI frame.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atupdatescreen(f, unregister) end

---Calls `f` with a platform-agnostic painter after every VI frame.
---The painter is only valid for the duration of the callback. Resources such as brushes, images, and text styles may be retained and reused across frames.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(p: Painter): nil The function to be called after every VI frame.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atpaint(f, unregister) end

---Calls the function `f` every input frame.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(controller: integer): nil The function to be called every input frame. `controller` is the controller index (0-based!).
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atinput(f, unregister) end

---Calls the function `f` when the script is stopped.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when the script is stopped.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atstop(f, unregister) end

---Defines a handler function that is called when a window receives a message.
---The only message that can be received is WM_MOUSEWHEEL for compatibility.
---All other functionality as been deprecated.
---The message data is given to the function in 4 parameters.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(a: integer, b: integer, c: integer, d: integer): nil The function to be called when a window message is received. a: wnd, b: msg, c: wParam, d: lParam.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atwindowmessage(f, unregister) end

---Calls the function `f` constantly, even when the emulator is paused.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called constantly.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atinterval(f, unregister) end

---Calls the function `f` when a movie is played.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a movie is played.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atplaymovie(f, unregister) end

---Calls the function `f` when a movie is stopped.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a movie is stopped.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atstopmovie(f, unregister) end

---Calls the function `f` when a savestate is loaded.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a savestate is loaded.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atloadstate(f, unregister) end

---Calls the function `f` when a savestate is saved.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a savestate is saved.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atsavestate(f, unregister) end

---Calls the function `f` when the emulator is reset.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when the emulator is reset.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atreset(f, unregister) end

---Calls the function `f` when seek is completed.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when the seek is completed.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atseekcompleted(f, unregister) end

---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atwarpmodifystatuschanged(f, unregister) end

---Calls the function `f` when a keyboard event happens.
---Keyboard presses that trigger hotkeys take priority over this event.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(args: KeyEventArgs): nil The function to be called when a keyboard event happens.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atkey(f, unregister) end

---Calls the function `f` when a mouse event happens.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(args: MouseEventArgs): nil The function to be called when a mouse event happens.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atmouse(f, unregister) end

---Returns the number of VIs since the last movie was played.
---This should match the statusbar.
---If no movie has been played, it returns the number of VIs since the emulator was started, not reset.
---@nodiscard
---@return integer framecount The number of VIs since the last movie was played.
function emu.framecount() end

---Returns the number of input frames since the last movie was played.
---This should match the statusbar.
---If no movie is playing, it will return the last value when a movie was playing.
---If no movie has been played yet, it will return `-1`.
---@nodiscard
---@return integer samplecount The number of input frames since the last movie was played.
function emu.samplecount() end

---Returns the number of input frames that have happened since the emulator was
---started. It does not reset when a movie is started. Alias for `joypad.count`.
---@nodiscard
---@return integer inputcount The number of input frames that have happened since the emulator was started.
function emu.inputcount() end

---Returns the current mupen version.
---If `type` is 0, it will return the full version name (Mupen 64 0.0.0).
---If `type` is 1, it will return only the version number (0.0.0).
---@nodiscard
---@param type 0|1 Whether to get the full version (`0`) or the short version (`1`).
---@return string version The Mupen version.
function emu.getversion(type) end

---Pauses or unpauses the emulator. Note that the pause parameter is inverted for compatibility reasons.
---@param pause boolean False pauses the emulator and true resumes it.
---@return nil
function emu.pause(pause) end

---Returns `true` if the emulator is paused and `false` if it is not.
---@nodiscard
---@return boolean emu_paused `true` if the emulator is paused and `false` if it is not.
function emu.getpause() end

---Returns the current speed limit (not the current speed) of the emulator.
---@nodiscard
---@return integer speed_limit The current speed limit of the emulator.
function emu.getspeed() end

---Gets the speed mode.
---@return CoreSpeedMode
function emu.get_speed_mode() end

---Sets the speed mode.
---@param mode CoreSpeedMode The speed mode to set.
function emu.set_speed_mode(mode) end

---Sets the speed limit of the emulator.
---@param speed_limit integer The new speed limit of the emulator.
---@return nil
function emu.speed(speed_limit) end

---Sets the speed mode of the emulator. Use [emu.setff](lua://emu.set_ff) instead.
---@deprecated Use emu.setff instead.
---@param mode "normal"|"maximum"
---@return nil
function emu.speedmode(mode) end

---@alias addresses
---|"rdram"
---|"rdram_register"
---|"MI_register"
---|"pi_register"
---|"sp_register"
---|"rsp_register"
---|"si_register"
---|"vi_register"
---|"ri_register"
---|"ai_register"
---|"dpc_register"
---|"dps_register"
---|"SP_DMEM"
---|"PIF_RAM"

---Gets the address of an internal mupen variable.
---For example, "rdram" is the same as mupen's ram start.
---@nodiscard
---@param address addresses
---@return integer
function emu.getaddress(address) end

---Takes a screenshot and saves it to the directory `dir`.
---@param dir string The directory to save the screenshot to.
---@return nil
function emu.screenshot(dir) end

---Played the sound file at `file_path`.
---@param file_path string
---@return nil
function emu.play_sound(file_path) end

---Returns `true` if the main mupen window is focused and false if it is not.
---@nodiscard
---@return boolean focused
function emu.ismainwindowinforeground() end

--#endregion


-- memory functions
--#region

---A representation of an 8 byte integer (quad word) as two 4 byte integers.
---@alias qword [integer, integer]

---Reinterprets the bits of a 4 byte integer `n` as a float and returns it.
---@nodiscard
---@param n integer
---@return number
function memory.inttofloat(n) end

---Reinterprets the bits of an 8 byte integer `n` as a double and returns it.
---@nodiscard
---@param n qword
---@return number
function memory.inttodouble(n) end

---Reinterprets the bits of a float `n` as a 4 byte integer and returns it.
---@nodiscard
---@param n number
---@return integer
function memory.floattoint(n) end

---Reinterprets the bits of a 8 byte integer `n` as a double and returns it.
---@nodiscard
---@param n qword
---@return number
function memory.doubletoint(n) end

---Takes in an 8 byte integer as a table of two 4 bytes integers and returns it as a lua number.
---@nodiscard
---@param n qword
---@return number
function memory.qwordtonumber(n) end

---Reads a signed byte from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return integer
function memory.readbytesigned(address) end

---Reads an unsigned byte from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return integer
function memory.readbyte(address) end

---Reads a signed word (2 bytes) from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return integer
function memory.readwordsigned(address) end

---Reads an unsigned word (2 bytes) from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return integer
function memory.readword(address) end

---Reads a signed dword (4 bytes) from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return integer
function memory.readdwordsigned(address) end

---Reads an unsigned dword (4 bytes) from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return integer
function memory.readdword(address) end

---Reads a signed qword (8 bytes) from memory at `address` and returns it as a table of the upper and lower 4 bytes.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return qword
function memory.readqwordsigned(address) end

---Reads an unsigned qword (8 bytes) from memory at `address` and returns it as a table of the upper and lower 4 bytes.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return [integer, integer]
function memory.readqword(address) end

---Reads a float (4 bytes) from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return number
function memory.readfloat(address) end

---Reads a double (8 bytes) from memory at `address` and returns it.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@return number
function memory.readdouble(address) end

---Reads `size` bytes from memory at `address` and returns them.
---The memory is treated as signed if `size` is is negative.
---Errors if the address is out of bounds.
---@nodiscard
---@param address integer
---@param size 1|2|4|8|-1|-2|-4|-8
---@return integer|qword
function memory.readsize(address, size) end

---Writes an unsigned byte to memory at `address`.
---Errors if the address is out of bounds.
---@param address integer
---@param data integer
---@return nil
function memory.writebyte(address, data) end

---Writes an unsigned word (2 bytes) to memory at `address`.
---Errors if the address is out of bounds.
---@param address integer
---@param data integer
---@return nil
function memory.writeword(address, data) end

---Writes an unsigned dword (4 bytes) to memory at `address`.
---Errors if the address is out of bounds.
---@param address integer
---@param data integer
---@return nil
function memory.writedword(address, data) end

---Writes an unsigned qword consisting of a table with the upper and lower 4 bytes to memory at `address`.
---Errors if the address is out of bounds.
---@param address integer
---@param data qword
---@return nil
function memory.writeqword(address, data) end

---Writes a float to memory at `address`.
---Errors if the address is out of bounds.
---@param address integer
---@param data number
---@return nil
function memory.writefloat(address, data) end

---Writes a double to memory at `address`.
---Errors if the address is out of bounds.
---@param address integer
---@param data number
---@return nil
function memory.writedouble(address, data) end

---Writes `size` bytes to memory at `address`.
---The memory is treated as signed if `size` is is negative.
---Errors if the address is out of bounds.
---@param address integer
---@param size 1|2|4|8|-1|-2|-4|-8
---@param data integer|qword
---@return nil
function memory.writesize(address, size, data) end

---Queues up a recompilation of the block at the specified address.
---@param addr integer
function memory.recompile(addr) end

---Queues up a recompilation of all blocks.
function memory.recompilenextall() end

--#endregion


-- debugger functions
--#region

---@alias BreakpointId integer

---@alias BreakpointCallback fun(state: CPUState): nil

---Places a breakpoint at the specified address.
---The emulated processor won't pause when it reaches this address.
---This function can only be called outside a breakpoint callback.
---@param address integer The address to place the breakpoint at.
---@param callback BreakpointCallback The callback function to call when the breakpoint is hit.
---@return BreakpointId
function debugger.add_breakpoint(address, callback) end

---Removes a breakpoint.
---@param id BreakpointId The ID of the breakpoint to remove.
function debugger.remove_breakpoint(id) end

---Disassembles an instruction based on a CPU state.
---@param state CPUState The CPU state to disassemble an instruction from.
---@return string The disassembled instruction.
function debugger.disassemble(state) end

--#endregion


-- wgui functions
--#region

---colors can be any of these or "#RGB", "#RGBA", "#RRGGBB", or "#RRGGBBA"
---@alias color
---| string
---| "white"
---| "black"
---| "clear"
---| "gray"
---| "red"
---| "orange"
---| "yellow"
---| "chartreuse"
---| "green"
---| "teal"
---| "cyan"
---| "blue"
---| "purple"

---@alias getrect {l: integer, t: integer, r: integer, b: integer}|{l: integer, t: integer, w: integer, h: integer}

---Sets the current GDI brush color to `color`.
---@param color color
function wgui.setbrush(color) end

---GDI: Sets the current GDI pen color to `color`.
---@param color color
---@param width number?
function wgui.setpen(color, width) end

---GDI: Sets the current GDI text color to `color`.
---@param color color
function wgui.setcolor(color) end

---GDI: Sets the current GDI background color to `color`.
---@param color color
function wgui.setbk(color) end

---GDI: Sets the font, font size, and font style.
---@param size integer? The size of the font. Defaults to 0 if not given
---@param font string? The name of the font from the operating system. Dafaults to "MS Gothic" if not given.
---@param style string? Each character in this string sets one style of the font, applied in chronological order. `b` sets bold, `i` sets italics, `u` sets underline, `s` sets strikethrough, and `a` sets antialiased. Defaults to "" if not given.
function wgui.setfont(size, font, style) end

---GDI: Displays text in one line with the current GDI background color and GDI text color.
---Use [`wgui.drawtext`](lua://wgui.drawtext) instead.
---@deprecated Use `wgui.drawtext` instead.
---@param x integer
---@param y integer
---@param text string
function wgui.text(x, y, text) end

---GDI: Draws text in the specified rectangle and with the specified format.
---@param text string The text to be drawn.
---@param rect getrect The rectangle in which to draw the text.
---@param format string? The format of the text. Applied in order stated. "l" aligns the text to the left (applied by default). "r" aligns the text to the right. "t" aligns text to the right (applied by default). "b" aligns text to the bottom. "c" horizontally aligns text. "v" vertically aligns the text. "e" adds ellipses if a line cannof fit all text. "s" forces text to be displayed on a single line.
function wgui.drawtext(text, rect, format) end

---Uses an alternate function for drawing text.
---Use [`wgui.drawtext`](lua://wgui.drawtext) instead.
---@deprecated Use `wgui.drawtext` unless you have a good reason.
---@param text string
---@param format integer
---@param left integer
---@param top integer
---@param right integer
---@param bottom integer
function wgui.drawtextalt(text, format, left, top, right, bottom) end

---Gets the width and height of the given text.
---@param text string
---@return {width: integer, height: integer}
function wgui.gettextextent(text) end

---GDI: Draws a rectangle at the specified coordinates with the current GDI background color and a border of the GDI pen color.
---Only use this function if you need rounded corners.
---Otherwise, use [`wgui.fillrecta`](lua://wgui.fillrecta).
---@param left integer
---@param top integer
---@param right integer
---@param bottom integer
---@param rounded_width integer? The width of the ellipse used to draw the rounded corners.
---@param rounded_height integer? The height of the ellipse used to draw the rounded corners.
function wgui.rect(left, top, right, bottom, rounded_width, rounded_height) end

---Draws a rectangle at the specified coordinates with the specified color.
---Use [`wgui.fillrecta`](lua://wgui.fillrecta) instead.
---@deprecated Use `wgui.fillrecta`.
---@param left integer
---@param top integer
---@param right integer
---@param bottom integer
---@param red integer
---@param green integer
---@param blue integer
function wgui.fillrect(left, top, right, bottom, red, green, blue) end

---GDIPlus: Draws a rectangle at the specified coordinates, size and color.
---@param x integer
---@param y integer
---@param w integer
---@param h integer
---@param color color|string Color names are currently broken
function wgui.fillrecta(x, y, w, h, color) end

---GDIPlus: Draws an ellipse at the specified coordinates, size, and color.
---@param x integer
---@param y integer
---@param w integer
---@param h integer
---@param color color|string Color names are currently broken
function wgui.fillellipsea(x, y, w, h, color) end

---Draws a filled in polygon using the points in `points`
---@param points [integer, integer][] Ex: `{{x1, y1}, {x2, y2}, {x3, y3}}`
---@param color color|string Color names are currently broken
function wgui.fillpolygona(points, color) end

---Loads an image file from `path` and returns the identifier of that image
---@param path string
---@return integer
function wgui.loadimage(path) end

---Deletes one or all images.
---@param idx integer The identifier of the image to clear. If it is 0, deletes all images.
function wgui.deleteimage(idx) end

---Saves an image to the specified path.
---@param idx integer The identifier of the image to save.
---@param path string The path to save the image to. The file extension determines the file format.
function wgui.saveimage(idx, path) end

---Draws the image at index `idx` at the specified coordinates.
---@param idx integer
---@param x integer
---@param y integer
function wgui.drawimage(idx, x, y) end

---Draws the image at index `idx` at the specified coordinates and scale.
---@param idx integer
---@param x integer
---@param y integer
---@param s number
function wgui.drawimage(idx, x, y, s) end

---Draws the image at index `idx` at the specified coordinates and size.
---@param idx integer
---@param x integer
---@param y integer
---@param w integer
---@param h integer
function wgui.drawimage(idx, x, y, w, h) end

---Draws the image at index `idx` at the specified coordinates, size, and rotation, using a part of the source image given by the `src` parameters.
---@param idx integer
---@param x integer
---@param y integer
---@param w integer
---@param h integer
---@param srcx integer
---@param srcy integer
---@param srcw integer
---@param srch integer
---@param rotate number
function wgui.drawimage(idx, x, y, w, h, srcx, srcy, srcw, srch, rotate) end

---Captures the current screen and saves it as an image.
---@return integer id The identifier of the saved image.
function wgui.loadscreen() end

---Re-initializes loadscreen.
function wgui.loadscreenreset() end

---Returns the width and height of the image at `idx`.
---@param idx integer
---@return {width: integer, height: integer}
function wgui.getimageinfo(idx) end

---Draws an ellipse at the specified coordinates and size.
---Uses the GDI brush color for the background and a border of the GDI pen color.
---@param left integer
---@param top integer
---@param right integer
---@param bottom integer
function wgui.ellipse(left, top, right, bottom) end

---Draws a polygon with the given points.
---Uses the GDI brush color for the background and a border of the GDI pen color.
---@param points integer[][]
function wgui.polygon(points) end

---Draws a line from `(x1, y1)` to `(x2, y2)`.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
function wgui.line(x1, y1, x2, y2) end

---Returns the current width and height of the mupen window in a table.
---@return {width: integer, height: integer}
function wgui.info() end

---Resizes the mupen window to `w` x `h`
---@param w integer
---@param h integer
function wgui.resize(w, h) end

---Sets a rectangle bounding box such that you cannot draw outside of it.
---@param x integer
---@param y integer
---@param w integer
---@param h integer
function wgui.setclip(x, y, w, h) end

---Resets the clip
function wgui.resetclip() end

--#endregion


-- painter functions
--#region

---All painter coordinates and sizes are expressed in device-independent pixels.
---@class PainterRect
---@field x number The x-coordinate of the top-left corner.
---@field y number The y-coordinate of the top-left corner.
---@field width number The rectangle width.
---@field height number The rectangle height.

---An RGBA color table.
---@class PainterColorTable
---@field r number The red component in the range [0, 1].
---@field g number The green component in the range [0, 1].
---@field b number The blue component in the range [0, 1].
---@field a number? The alpha component in the range [0, 1]. Defaults to 1.

---A color, either an RGBA table or a hex string `"#RRGGBBAA"` or `"#RRGGBB"`.
---@alias PainterColor
---| PainterColorTable
---| string

---A fill or stroke paint source: either a reusable [PainterBrush](lua://PainterBrush) or a [PainterColor](lua://PainterColor).
---@alias PainterFill
---| PainterBrush
---| PainterColor

---A flat list of coordinates in the form `{ x1, y1, x2, y2, ... }`.
---A flat representation avoids allocating a table for every point and must contain at least two points.
---@alias PainterPoints number[]

---@alias PainterLineCap "butt"|"round"|"square"
---@alias PainterLineJoin "miter"|"round"|"bevel"

---@class PainterStrokeStyle
---@field width number? The stroke width. Defaults to 1.
---@field cap PainterLineCap? The shape of line endpoints. Defaults to `"butt"`.
---@field join PainterLineJoin? The shape of line joins. Defaults to `"miter"`.
---@field miter_limit number? The maximum miter length relative to the stroke width. Defaults to 4.
---@field dashes number[]? Alternating dash and gap lengths. An empty or absent list produces a solid stroke.
---@field dash_offset number? The offset into the dash pattern. Defaults to 0.

---@alias PainterFontSlant "normal"|"italic"|"oblique"

---@class PainterTextStyleParams
---@field family string|string[]? A font family or fallback list. Defaults to the platform UI font.
---@field size number? The font size. Defaults to 12.
---@field weight integer? A font weight from 1 through 1000. 400 is normal and 700 is bold. Defaults to 400.
---@field slant PainterFontSlant? Defaults to `"normal"`.
---@field underline boolean? Defaults to false.
---@field strikethrough boolean? Defaults to false.
---@field letter_spacing number? Extra spacing between characters. Defaults to 0.
---@field line_height number? Line height as a multiplier of the font size. If absent, the font's natural line height is used.

---@alias PainterTextHorizontalAlign "left"|"center"|"right"|"justify"
---@alias PainterTextVerticalAlign "top"|"center"|"bottom"
---@alias PainterTextWrap "none"|"word"|"character"
---@alias PainterTextOverflow "visible"|"clip"|"ellipsis"

---@class PainterTextLayout
---@field align_x PainterTextHorizontalAlign? Horizontal alignment. Defaults to `"left"`.
---@field align_y PainterTextVerticalAlign? Vertical alignment. Defaults to `"top"`.
---@field wrap PainterTextWrap? Wrapping mode. Defaults to `"word"`.
---@field overflow PainterTextOverflow? Behavior when text does not fit. Defaults to `"clip"`.
---@field clip boolean? Whether glyphs are clipped to the layout rectangle. Defaults to true.

---@class PainterTextConstraints
---@field width number? Maximum layout width. If absent, width is unconstrained.
---@field height number? Maximum layout height. If absent, height is unconstrained.
---@field wrap PainterTextWrap? Wrapping mode. Defaults to `"word"` when `width` is present and `"none"` otherwise.
---@field max_lines integer? Maximum number of laid-out lines.

---@class PainterTextMetrics
---@field width number The width of the laid-out text, including trailing whitespace.
---@field height number The height of the laid-out text.
---@field line_count integer The number of laid-out lines.
---@field baseline number The first line's baseline measured from the top of the layout.
---@field truncated boolean Whether width, height, or `max_lines` truncated the text.

---@alias PainterSampling "nearest"|"linear"

---@class PainterImageOptions
---@field source PainterRect? The source rectangle in image pixels. Defaults to the whole image. Required when `center` is provided.
---@field center PainterRect? The center rectangle in image pixels for nine-sliced drawing. It must be contained by `source`; the surrounding corners retain their original size and aspect ratio. If the destination cannot fit the corners, only this center is stretched over it.
---@field opacity number? Opacity in the range [0, 1]. Defaults to 1.
---@field sampling PainterSampling? Sampling used when scaling. Defaults to `"linear"`.
---@field tint PainterColor? A color multiplied with the image pixels before blending.

---An immutable, reusable solid-color brush.
---Resources are garbage-collected, but `close` can be used for deterministic release.
---@class PainterBrush
local PainterBrush = {}

---Releases the brush's native resources. Calling this more than once has no effect.
---Using the brush afterward is an error.
function PainterBrush:close() end

---A decoded image which can also be used as a drawing target.
---Resources are garbage-collected, but `close` can be used for deterministic release.
---@class PainterImage
---@field width integer The natural width in pixels.
---@field height integer The natural height in pixels.
local PainterImage = {}

---Invokes `callback` immediately with a painter targeting this image.
---Drawing updates the image in place. The painter is only valid for the duration of the callback.
---@param callback fun(p: Painter): nil
function PainterImage:paint(callback) end

---Releases the image's native resources. Calling this more than once has no effect.
---Using the image afterward is an error.
function PainterImage:close() end

---An immutable, reusable text style.
---Resources are garbage-collected, but `close` can be used for deterministic release.
---@class PainterTextStyle
local PainterTextStyle = {}

---Releases the text style's native resources. Calling this more than once has no effect.
---Using the style afterward is an error.
function PainterTextStyle:close() end

---Creates a reusable solid-color brush.
---@nodiscard
---@param color PainterColor
---@return PainterBrush
function painter.brush(color) end

---Creates a reusable text style.
---@nodiscard
---@param params PainterTextStyleParams
---@return PainterTextStyle
function painter.text_style(params) end

---Creates a transparent image which can be drawn into with [PainterImage:paint](lua://PainterImage.paint).
---@nodiscard
---@param width integer The image width in pixels. Must be greater than zero.
---@param height integer The image height in pixels. Must be greater than zero.
---@return PainterImage
function painter.new_image(width, height) end

---Loads and decodes an image from a file.
---The supported encoded formats are implementation-defined and can be queried with [painter.image_formats](lua://painter.image_formats).
---@nodiscard
---@param path string
---@return PainterImage? image
---@return string? error_message
function painter.load_image(path) end

---Decodes an image from a binary Lua string.
---The encoded format is detected from the data.
---@nodiscard
---@param data string
---@return PainterImage? image
---@return string? error_message
function painter.decode_image(data) end

---Returns the lowercase names of encoded image formats supported by this implementation, such as `"png"` and `"jpeg"`.
---@nodiscard
---@return string[]
function painter.image_formats() end

---The short-lived drawing context supplied to [emu.atpaint](lua://emu.atpaint).
---Methods must only be called while the callback which supplied this object is active.
---@class Painter
local Painter = {}

---Clears the entire drawing target to `color`, ignoring the current clip stack.
---When called from [PainterImage:paint](lua://PainterImage.paint), this clears the image rather than the screen.
---@param color PainterColor
function Painter:clear(color) end

---Fills `rect` with `paint`.
---@param rect PainterRect
---@param paint PainterFill
function Painter:fill_rect(rect, paint) end

---Strokes the inside edge of `rect`.
---@param rect PainterRect
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:stroke_rect(rect, paint, style) end

---Fills a rectangle with uniformly rounded corners.
---@param rect PainterRect
---@param radius number The corner radius. Values are clamped to fit the rectangle.
---@param paint PainterFill
function Painter:fill_round_rect(rect, radius, paint) end

---Strokes a rectangle with uniformly rounded corners.
---@param rect PainterRect
---@param radius number The corner radius. Values are clamped to fit the rectangle.
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:stroke_round_rect(rect, radius, paint, style) end

---Fills the ellipse inscribed in `rect`.
---@param rect PainterRect
---@param paint PainterFill
function Painter:fill_ellipse(rect, paint) end

---Strokes the ellipse inscribed in `rect`.
---@param rect PainterRect
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:stroke_ellipse(rect, paint, style) end

---Fills a circle.
---@param x number The center x-coordinate.
---@param y number The center y-coordinate.
---@param radius number
---@param paint PainterFill
function Painter:fill_circle(x, y, radius, paint) end

---Strokes a circle.
---@param x number The center x-coordinate.
---@param y number The center y-coordinate.
---@param radius number
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:stroke_circle(x, y, radius, paint, style) end

---Draws a line segment.
---@param x1 number
---@param y1 number
---@param x2 number
---@param y2 number
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:line(x1, y1, x2, y2, paint, style) end

---Draws connected line segments without closing the shape.
---@param points PainterPoints
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:polyline(points, paint, style) end

---Fills a closed polygon using the non-zero winding rule.
---@param points PainterPoints
---@param paint PainterFill
function Painter:fill_polygon(points, paint) end

---Strokes a closed polygon.
---@param points PainterPoints
---@param paint PainterFill
---@param style PainterStrokeStyle?
function Painter:stroke_polygon(points, paint, style) end

---Draws an image into `destination`.
---When `options.center` is provided, the image is drawn in nine slices. Corners remain unscaled, edges scale along one axis, and the center scales along both axes.
---@param image PainterImage
---@param destination PainterRect
---@param options PainterImageOptions?
function Painter:image(image, destination, options) end

---Draws laid-out text inside `rect`.
---Explicit newline characters always start a new line. Text shaping, bidirectional text, and font fallback are implementation responsibilities.
---@param text string
---@param rect PainterRect
---@param style PainterTextStyle
---@param paint PainterFill
---@param layout PainterTextLayout?
function Painter:text(text, rect, style, paint, layout) end

---Measures text using the same shaping and wrapping rules as [Painter:text](lua://Painter.text).
---No drawing context is required, so the result may be cached by an implementation.
---@nodiscard
---@param text string
---@param style PainterTextStyle
---@param constraints PainterTextConstraints?
---@return PainterTextMetrics
function painter.measure_text(text, style, constraints) end

---Intersects `rect` with the current clip and pushes the result onto a stack.
---Clips are scoped to this painter and are automatically discarded when its callback returns.
---@param rect PainterRect
function Painter:push_clip(rect) end

---Removes the most recently pushed clip. Calling this without a matching `push_clip` is an error.
function Painter:pop_clip() end

--#endregion


-- input functions
--#region


---@alias Keys {
---leftclick: boolean?,
---rightclick: boolean?,
---middleclick: boolean?,
---backspace: boolean?,
---tab: boolean?,
---enter: boolean?,
---shift: boolean?,
---control: boolean?,
---alt: boolean?,
---pause: boolean?,
---capslock: boolean?,
---escape: boolean?,
---space: boolean?,
---pageup: boolean?,
---pagedown: boolean?,
---end: boolean?,
---home: boolean?,
---left: boolean?,
---up: boolean?,
---right: boolean?,
---down: boolean?,
---insert: boolean?,
---delete: boolean?,
---["0"]: boolean?,
---["1"]: boolean?,
---["2"]: boolean?,
---["3"]: boolean?,
---["4"]: boolean?,
---["5"]: boolean?,
---["6"]: boolean?,
---["7"]: boolean?,
---["8"]: boolean?,
---["9"]: boolean?,
---A: boolean?,
---B: boolean?,
---C: boolean?,
---D: boolean?,
---E: boolean?,
---F: boolean?,
---G: boolean?,
---H: boolean?,
---I: boolean?,
---J: boolean?,
---K: boolean?,
---L: boolean?,
---M: boolean?,
---N: boolean?,
---O: boolean?,
---P: boolean?,
---Q: boolean?,
---R: boolean?,
---S: boolean?,
---T: boolean?,
---U: boolean?,
---V: boolean?,
---W: boolean?,
---X: boolean?,
---Y: boolean?,
---Z: boolean?,
---numpad0: boolean?,
---numpad1: boolean?,
---numpad2: boolean?,
---numpad3: boolean?,
---numpad4: boolean?,
---numpad5: boolean?,
---numpad6: boolean?,
---numpad7: boolean?,
---numpad8: boolean?,
---numpad9: boolean?,
---numpad*: boolean?,
---["numpad+"]: boolean?,
---numpad-: boolean?,
---numpad.: boolean?,
---["numpad/"]: boolean?,
---F1: boolean?,
---F2: boolean?,
---F3: boolean?,
---F4: boolean?,
---F5: boolean?,
---F6: boolean?,
---F7: boolean?,
---F8: boolean?,
---F9: boolean?,
---F10: boolean?,
---F11: boolean?,
---F12: boolean?,
---F13: boolean?,
---F14: boolean?,
---F15: boolean?,
---F16: boolean?,
---F17: boolean?,
---F18: boolean?,
---F19: boolean?,
---F20: boolean?,
---F21: boolean?,
---F22: boolean?,
---F23: boolean?,
---F24: boolean?,
---numlock: boolean?,
---scrolllock: boolean?,
---semicolon: boolean?,
---plus: boolean?,
---comma: boolean?,
---minus: boolean?,
---period: boolean?,
---slash: boolean?,
---tilde: boolean?,
---leftbracket: boolean?,
---backslash: boolean?,
---rightbracket: boolean?,
---quote: boolean?,
---xmouse: integer,
---ymouse: integer,
---ywmouse: integer,
---}

---Returns the state of all keyboard keys and the mouse position in a table.
---Ex: `input.get() -> {xmouse=297, ymouse=120, A=true, B=true}`.
---@nodiscard
---@return Keys
function input.get() end

---Returns the differences between `t1` and `t2`.
---For example, if `t1` is the inputs for this frame, and `t2` is the inputs for last frame, it would return which buttons were pressed this frame, not which buttons are active.
---@nodiscard
---@param t1 table
---@param t2 table
---@return table
function input.diff(t1, t2) end

---Opens a dialog in which the user can input text.
---If the dialog is cancelled, `nil` is returned.
---@nodiscard
---@param title string? The title of the text box. Defaults to `"input:"`.
---@param placeholder string? The text box is filled with this string when it opens. Defaults to `""`.
---@return string|nil
function input.prompt(title, placeholder) end

---Gets the name of a key.
---@nodiscard
---@param key Keycode
---@return string
function input.get_key_name_text(key) end

--#endregion


-- joypad functions
--#region

---@class JoypadInputs
---@field right boolean
---@field left boolean
---@field down boolean
---@field up boolean
---@field start boolean
---@field Z boolean
---@field B boolean
---@field A boolean
---@field Cright boolean
---@field Cleft boolean
---@field Cdown boolean
---@field Cup boolean
---@field R boolean
---@field L boolean
---@field X integer The joystick X value with range [-128, 127].
---@field Y integer The joystick Y value with range [-128, 127].


---Gets the currently pressed game buttons and stick direction for a given port.
---If `port` is nil, the data for port 1 will be returned.
---Note that the `y` coordinate of the stick is the opposite of what is shown on TAS Input.
---@nodiscard
---@param port? 1|2|3|4
---@return JoypadInputs
function joypad.get(port) end

---Sets the input state for port 0 to `inputs`.
---If you do not specify one or more inputs, they will be set to `false` for buttons or `0` for stick coordinates.
---@param inputs JoypadInputs
function joypad.set(inputs) end

---Sets the input state for a given port to `inputs`.
---If you do not specify one or more inputs, they will be set to `false` for buttons or `0` for stick coordinates.
---@param port 1|2|3|4
---@param inputs JoypadInputs
---@return nil
function joypad.set(port, inputs) end

---Returns the number of input frames that have happened since the emulator was
---started. It does not reset when a movie is started. Alias for
---`emu.inputcount`.
---@nodiscard
---@return integer inputcount The number of input frames that have happened since the emulator was started.
function joypad.count() end

--#endregion


-- movie functions
--#region

---Plays a movie.
---This function sets `Read Only` to true.
---@param path string
---@return Result # The operation result.
function movie.play(path) end

---Stops the currently playing movie.
---@return nil
function movie.stop() end

---Returns the filename of the currently playing movie.
---It will error if no movie is playing.
---@nodiscard
---@return string
function movie.get_filename() end

---Returns true if the currently playing movie is read only.
---@nodiscard
---@return boolean
function movie.get_readonly() end

---Set's the currently movie's readonly state to `readonly`.
---@param readonly boolean
function movie.set_readonly(readonly) end

---Begins seeking.
---The seek operation might end before the target frame is reached, so make sure to check the current frame via `movie.get_seek_completion()`.
---@param str string
---@param pause_at_end boolean
---@return integer
function movie.begin_seek(str, pause_at_end) end

---Stops seeking.
function movie.stop_seek() end

---Returns whether the emulator is currently seeking.
---@return boolean
function movie.is_seeking() end

---Gets info about the current seek.
---@return [integer, integer]
function movie.get_seek_completion() end

---Begins a warp modification operation. A "warp modification operation" is the changing of sample data which is
---temporally behind the current sample.
---The VCR engine will find the last common sample between the current input buffer and the provided one.
---Then, the closest savestate prior to that sample will be loaded and recording will be resumed with the
---modified inputs up to the sample the function was called at.
---This operation is long-running and status is reported via the WarpModifyStatusChanged message.
---A successful warp modify operation can be detected by the status changing from warping to none with no errors
---inbetween.
---If the provided buffer is identical to the current input buffer (in both content and size), the operation
---will succeed with no seek.
---If the provided buffer is larger than the current input buffer and the first differing input is after the
---current sample, the operation will succeed with no seek. The input buffer will be overwritten with the
---provided buffer and when the modified frames are reached in the future, they will be "applied" like in
---playback mode.
---If the provided buffer is smaller than the current input buffer, the VCR engine will seek to the last frame
---and otherwise perform the warp modification as normal.
---An empty input buffer will cause the operation to fail.
---@param inputs JoypadInputs[] The new input buffer to use for the warp modification. Note that the X/Y joystick values are mismatched: X is Y and Y is X. This behavior is kept for backwards compatibility.
---@return Result # The operation result.
function movie.begin_warp_modify(inputs) end

--#endregion


-- savestate functions
--#region

---A byte buffer encoded as a string.
---@alias ByteBuffer string

---Represents a callback function for a savestate operation.
---@alias SavestateCallback fun(result: Result, data: ByteBuffer): nil

---Represents a savestate job.
---@alias SavestateJob "save" | "load"

---Executes a savestate operation to a path.
---@param path string The savestate's path.
---@param job SavestateJob The job to set.
---@param callback SavestateCallback The callback to call when the operation is complete.
---@param ignore_warnings boolean | nil Whether warnings, such as those about ROM compatibility, shouldn't be shown. Defaults to `false`.
function savestate.do_file(path, job, callback, ignore_warnings) end

---Executes a savestate operation to a slot.
---@param slot integer The slot to construct the savestate path with.
---@param job SavestateJob The job to set.
---@param callback SavestateCallback The callback to call when the operation is complete.
---@param ignore_warnings boolean | nil Whether warnings, such as those about ROM compatibility, shouldn't be shown. Defaults to `false`.
function savestate.do_slot(slot, job, callback, ignore_warnings) end

---Executes a savestate operation in-memory.
---@param buffer ByteBuffer The buffer to use for the operation. If the `job` is `save`, this parameter is ignored.
---@param job SavestateJob The job to set.
---@param callback SavestateCallback The callback to call when the operation is complete.
---@param ignore_warnings boolean | nil Whether warnings, such as those about ROM compatibility, shouldn't be shown. Defaults to `false`.
function savestate.do_memory(buffer, job, callback, ignore_warnings) end

--#endregion


-- iohelper functions
--#region

---Opens a file dialouge and returns the file path of the file chosen.
---@nodiscard
---@param filter string This string acts as a filter for what files can be chosen. For example `*.*` selects all files, where `*.txt` selects only text files.
---@param type integer 0 for an open file dialog. 1 for a save file dialog.
---@return string
function iohelper.filediag(filter, type) end

--#endregion


-- avi functions
--#region

---Begins an avi recording using the previously saved capture settings.
---It is saved to `filename`.
---@param filename string
---@return nil
function avi.startcapture(filename) end

---Stops avi recording.
---@return nil
function avi.stopcapture() end

--#endregion


-- hotkey functions
--#region

---@class HotkeyNoTrigger
---@field type "none"
---Represents an unassigned hotkey trigger.

---@class HotkeyKeyCodeTrigger
---@field type "keycode"
---@field value Keycode The SDL keycode that triggers the hotkey.
---Represents a keyboard hotkey trigger.

---@class HotkeyMouseButtonTrigger
---@field type "mousebutton"
---@field value MouseButtonFlags The SDL mouse-button flag that triggers the hotkey.
---Represents a mouse hotkey trigger.

---@alias HotkeyTrigger HotkeyNoTrigger|HotkeyKeyCodeTrigger|HotkeyMouseButtonTrigger

---@class Hotkey
---@field trigger HotkeyTrigger The event that triggers the hotkey.
---@field ctrl boolean? Whether the control modifier is pressed. Defaults to `false`.
---@field shift boolean? Whether the shift modifier is pressed. Defaults to `false`.
---@field alt boolean? Whether the alt modifier is pressed. Defaults to `false`.
---Represents a trigger and its keyboard modifiers. Can invoke an action.

---Shows a dialog prompting the user to enter a hotkey.
---@param caption string The headline to display in the dialog.
---@return Hotkey|nil The hotkey that was entered, or `nil` if the user cancelled the dialog.
function hotkey.prompt(caption) end

--#endregion


-- action functions
--#region

---@alias ActionFilter string
---An action filter that can be used to match actions in the action registry.
---Can be in the format `[Category[] | *] > [Name | *]`.
---The `*` wildcard can be used to match any child from that segment onwards.
---The wildcard must always be the last segment in the filter: wildcard-based wide lookups like `A > * > C` aren't supported.
---Example queries:
---`*` - matches all actions.
---`Mupen64 > File > *` - matches "Mupen64 > File > Load ROM...", "Mupen64 > File > Recent ROMs > Load Recent Item #5", etc...
---`Mupen64 > File` - matches nothing, because `File` has no action associated with it.

---@alias ActionPath string
---A fully-qualified action path in the format `"Category[] > Name"`.
---An action path is a subset of the action filter that contains no wildcards and is used to uniquely identify an action.

---@alias ActionArgumentMap { [string]: string }
---Represents a collection of action parameter keys to their values.

---@class ActionParam
---@field key string The key of the parameter.
---@field name string The display name of the parameter.
---@field validator fun(value: string): string? A validator function that takes in a parameter value and optionally returns an error message if the validation failed.
---@field get_initial_value fun(): string? A function that returns the initial value of the parameter. Can be null.
---@field get_hints fun(value: string): string[]? A function that returns hints for the parameter based on the current input. Can be null.
---Represents an action parameter.

---@class ActionAddParams
---@field path ActionPath The action's path. If the path's final segment is prefixed with `#`, it won't be visible in the menu.
---@field params ActionParam[]? The action parameters.
---@field on_press fun(params: ActionArgumentMap)? The callback to be invoked when the action is pressed. If this action has parameters, they will be supplied as an argument map.
---@field on_release fun()? The callback to be invoked when the action is released. Can be null.
---@field get_display_name (fun(): string)? The function used to determine the function's display name. If null, the display name will be derived from the path.
---@field get_enabled (fun(): boolean)? The function used to determine whether the action is enabled. If null, the action will be considered enabled.
---@field get_active (fun(): boolean)? The function used to determine whether the action is "active". The active state usually means a checked or toggled UI state. If null, the action will be considered inactive.

---Adds an action to the action registry.
---If an action with the same path already exists, the operation will fail.
---If adding the action causes another action to gain a child (e.g. there's an action `A > B`, and we're adding `A > B > C > D`), the operation will fail. To add the action, delete the original action (`A > B`) first.
---@param params ActionAddParams The action parameters.
---@return boolean # Whether the operation succeeded.
function action.add(params) end

---Removes actions matching the specified filter.
---@param filter ActionFilter A filter.
---@return ActionPath[] # A collection containing the paths of the actions that were removed.
function action.remove(filter) end

---Associates a hotkey with an action by its path, while replacing any existing hotkey association for that action.
---@param path ActionPath A path.
---@param hotkey Hotkey The hotkey to associate with the action.
---@param overwrite_existing boolean? Whether the any existing hotkey association will be overwritten. If false, the hotkey will only be associated if the action has no hotkey associated with it already.
---@return boolean # Whether the operation succeeded.
function action.associate_hotkey(path, hotkey, overwrite_existing) end

---Begins a batch operation. Batches all updates caused by [action.add](lua://action.add), [action.remove](lua://action.remove), and [action.associate_hotkey](lua://action.associate_hotkey) into one at the succeeding call to [action.end_batch_work](lua://action.end_batch_work).
function action.begin_batch_work() end

---Ends a batch operation.
function action.end_batch_work() end

---Notifies about the display name of actions matching a filter changing.
---@param filter ActionFilter A filter.
function action.notify_display_name_changed(filter) end

---Notifies about the enabled state of actions matching a filter changing.
---@param filter ActionFilter A filter.
function action.notify_enabled_changed(filter) end

---Notifies about the active state of actions matching a filter changing.
---@param filter ActionFilter A filter.
function action.notify_active_changed(filter) end

---Gets the display name for a given filter.
---@param filter ActionFilter A filter.
---@param ignore_override boolean? Whether to ignore the display name override.
---@return string # The action's display name or an empty string if the display name couldn't be resolved.
function action.get_display_name(filter, ignore_override) end

---Gets whether an action is enabled.
---@param path ActionPath A path.
---@return boolean # The action's enabled state.
function action.get_enabled(path) end

---Gets whether an action is active.
---@param path ActionPath A path.
---@return boolean # The action's active state.
function action.get_active(path) end

---Gets whether an action has been registered with an active state callback.
---@param path ActionPath A path.
---@return boolean # The action's activatability.
function action.get_activatability(path) end

---Gets the parameters associated with an action.
---@param path ActionPath A path.
---@return ActionParam[] # The action's parameters.
function action.get_params(path) end

---Gets all action paths that match the specified filter.
---@param filter ActionFilter A filter.
---@return ActionPath[] # A collection of action paths that match the filter.
function action.get_actions_matching_filter(filter) end

---Manually invokes an action by its path. If the action has an up callback, is already pressed down, and `up` is false, only the up callback will be invoked.
---@param path ActionPath A path.
---@param up boolean? If true, the action is considered to be released, otherwise it is considered to be pressed down.
---@param release_on_repress boolean? If true, if the action is already pressed down and `up` is false, the action will first be released before being pressed down again. If false, the action will only be pressed down. Defaults to true.
---@param params ActionArgumentMap? The action parameters.
---@return boolean # Whether the operation succeeded.
function action.invoke(path, up, release_on_repress, params) end

---Locks or unlocks action invocations from hotkeys.
---@param lock boolean Whether to lock or unlock action invocations from hotkeys.
function action.lock_hotkeys(lock) end

---@return boolean # Whether action invocations from hotkeys are currently locked.
function action.get_hotkeys_locked() end

--#endregion


-- clipboard functions
--#region

---@alias ClipboardContentType "text"

---Gets the clipboard content as text.
---@param type ClipboardContentType["text"] The clipboard content type.
---@return string? The clipboard content in the desired type, or `nil` if the clipboard content isn't of the same type as requested.
function clipboard.get(type) end

---Gets the content type of the current clipboard contents.
---@return "text" | nil # The clipboard content type, or `nil` if the clipboard is empty.
function clipboard.get_content_type() end

---Sets the clipboard content to the specified text.
---@param type ClipboardContentType["text"] The clipboard content type.
---@param value string The new clipboard value.
---@return boolean # Whether the operation succeeded.
function clipboard.set(type, value) end

---Clears the clipboard.
---@return boolean # Whether the operation succeeded.
function clipboard.clear() end

--#endregion
