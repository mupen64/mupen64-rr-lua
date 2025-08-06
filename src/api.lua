---@meta

-- version 1.2.0.1

-- This file has meta definitions for the functions implemented in mupen64.
-- https://github.com/mupen64/mupen64-rr-lua/blob/master/src/Views.Win32/lua/LuaRegistry.cpp

-- Additional (and very outdated) documentation can be found here:
-- https://docs.google.com/document/d/1SWd-oAFBKsGmwUs0qGiOrk3zfX9wYHhi3x5aKPQS_o0

emu = {}
memory = {}
wgui = {}
d2d = {}
input = {}
joypad = {}
movie = {}
savestate = {}
iohelper = {}
avi = {}
hotkey = {}
action = {}

Mupen = {
    ---@enum Result
    ---An enum containing results that can be returned by the core.
    result = {
        -- The operation completed successfully
        res_ok = 0,

        -- The operation was cancelled by the user
        res_cancelled = 1,

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
        -- The provided start type is invalid.
        vcr_invalid_start_type = 14,
        -- Another warp modify operation is already running
        vcr_warp_modify_already_running = 15,
        -- Warp modifications can only be performed during recording
        vcr_warp_modify_needs_recording_task = 16,
        -- The provided input buffer is empty
        vcr_warp_modify_empty_input_buffer = 17,
        -- Another seek operation is already running
        vcr_seek_already_running = 18,
        -- The seek operation could not be initiated due to a savestate not being loaded successfully
        vcr_seek_savestate_load_failed = 19,
        -- The seek operation can't be initiated because the seek savestate interval is 0
        vcr_seek_savestate_interval_zero = 20,

        -- Couldn't find a rom matching the provided movie
        vr_no_matching_rom = 21,
        -- An error occured during plugin loading
        vr_plugin_error = 22,
        -- The ROM or alternative rom source is invalid
        vr_rom_invalid = 23,
        -- The emulator isn't running yet
        vr_not_running = 24,
        -- Failed to open core streams
        vr_file_open_failed = 25,

        -- The core isn't launched
        st_core_not_launched = 26,
        -- The savestate file wasn't found
        st_not_found = 27,
        -- The savestate couldn't be written to disk
        st_file_write_error = 28,
        -- Couldn't decompress the savestate
        st_decompression_error = 29,
        -- The event queue was too long
        st_event_queue_too_long = 30,
        -- The CPU registers contained invalid values
        st_invalid_registers = 31,

        -- The plugin library couldn't be loaded
        pl_load_library_failed = 32,
        -- The plugin doesn't export a GetDllInfo function
        pl_no_get_dll_info = 33,
    },

    ---@enum VKeycodes
    ---An enum containing virtual keycodes.
    VKeycodes = {
        VK_LBUTTON = 0x01, -- Left mouse button
        VK_RBUTTON = 0x02, -- Right mouse button
        VK_CANCEL = 0x03, -- Control‑break
        VK_MBUTTON = 0x04, -- Middle mouse button
        VK_XBUTTON1 = 0x05, -- X1 mouse button
        VK_XBUTTON2 = 0x06, -- X2 mouse button
        -- 0x07 Reserved
        VK_BACK = 0x08,  -- Backspace
        VK_TAB = 0x09,   -- Tab
        -- 0x0A–0B Reserved
        VK_CLEAR = 0x0C, -- Clear
        VK_RETURN = 0x0D, -- Enter
        -- 0x0E–0F Unassigned
        VK_SHIFT = 0x10, -- Shift
        VK_CONTROL = 0x11, -- Ctrl
        VK_MENU = 0x12,  -- Alt (Menu)
        VK_PAUSE = 0x13, -- Pause
        VK_CAPITAL = 0x14, -- Caps Lock
        VK_KANA = 0x15,  -- IME Kana / Hangul
        VK_IME_ON = 0x16, -- IME On
        VK_JUNJA = 0x17, -- IME Junja
        VK_FINAL = 0x18, -- IME Final
        VK_HANJA = 0x19, -- IME Hanja / Kanji
        VK_IME_OFF = 0x1A, -- IME Off
        VK_ESCAPE = 0x1B, -- Escape
        VK_CONVERT = 0x1C, -- IME Convert
        VK_NONCONVERT = 0x1D, -- IME Nonconvert
        VK_ACCEPT = 0x1E, -- IME Accept
        VK_MODECHANGE = 0x1F, -- IME Mode Change
        VK_SPACE = 0x20, -- Spacebar
        VK_PRIOR = 0x21, -- Page Up
        VK_NEXT = 0x22,  -- Page Down
        VK_END = 0x23,   -- End
        VK_HOME = 0x24,  -- Home
        VK_LEFT = 0x25,  -- Left Arrow
        VK_UP = 0x26,    -- Up Arrow
        VK_RIGHT = 0x27, -- Right Arrow
        VK_DOWN = 0x28,  -- Down Arrow
        VK_SELECT = 0x29, -- Select
        VK_PRINT = 0x2A, -- Print
        VK_EXECUTE = 0x2B, -- Execute
        VK_SNAPSHOT = 0x2C, -- Print Screen
        VK_INSERT = 0x2D, -- Insert
        VK_DELETE = 0x2E, -- Delete
        VK_HELP = 0x2F,  -- Help
        VK_LWIN = 0x5B,  -- Left Windows
        VK_RWIN = 0x5C,  -- Right Windows
        VK_APPS = 0x5D,  -- Applications (Menu) key
    }
}

---The `lua_tostring` c function converts numbers to strings, so numbers are
---acceptable to pass into some functions that use that function.
---@alias tostringusable string|number


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

---Similar to `emu.atvi`, but for wgui drawing commands.
---Only drawing functions from the wgui namespace will work here, those from d2d will not.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called after every VI frame.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atupdatescreen(f, unregister) end

---Similar to `emu.atvi`, but for d2d and wgui drawing commands.
---Drawing functions from both the d2d and wgui namespaces will work here, but it's recommended to put wgui drawcalls into the `emu.atupdatescreen` callback for efficiency and compatibility reasons.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called after every VI frame.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atdrawd2d(f, unregister) end

---Calls the function `f` every input frame.
---The function `f` receives an argument that seems to always be `0`.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(a: integer?): nil The function to be called every input frame. It receives an argument that seems to always be `0`.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atinput(f, unregister) end

---Calls the function `f` when the script is stopped.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when the script is stopped.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atstop(f, unregister) end

---Defines a handler function that is called when a window receives a message.
---The only message that can be received is WM_MOUSEWHEEL for compatibility.
---All other functionality as been deprecated.
---The message data is given to the function in 4 parameters.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(a: integer, b: integer, c: integer, d: integer): nil The function to be called when a window message is received. a: wnd, b: msg, c: wParam, d: lParam.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atwindowmessage(f, unregister) end

---Calls the function `f` constantly, even when the emulator is paused.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called constantly.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atinterval(f, unregister) end

---Calls the function `f` when a movie is played.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a movie is played.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atplaymovie(f, unregister) end

---Calls the function `f` when a movie is stopped.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a movie is stopped.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atstopmovie(f, unregister) end

---Calls the function `f` when a savestate is loaded.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a savestate is loaded.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atloadstate(f, unregister) end

---Calls the function `f` when a savestate is saved.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when a savestate is saved.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atsavestate(f, unregister) end

---Calls the function `f` when the emulator is reset.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when the emulator is reset.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atreset(f, unregister) end

---Calls the function `f` when seek is completed.
---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called when the seek is completed.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atseekcompleted(f, unregister) end

---Emulator execution may be running in parallel with this callback and it's therefore disallowed to call any functions that perform unsynchronized reads or writes from and to the emulator state, such as those from the memory namespace.
---If `unregister` is set to true, the function `f` will no longer be called when this event occurs, but it will error if you never registered the function.
---@param f fun(): nil The function to be called.
---@param unregister boolean? If true, then unregister the function `f`.
---@return nil
function emu.atwarpmodifystatuschanged(f, unregister) end

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

---Pauses or unpauses the emulator.
---@param pause boolean True pauses the emulator and false resumes it.
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

---Gets whether fast forward is active.
---@return boolean
function emu.get_ff() end

---Sets whether fast forward is active.
---@param fast_forward boolean
function emu.set_ff(fast_forward) end

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
---This does not convert from an int to a float, but reinterprets the memory.
---@nodiscard
---@param n integer
---@return number
function memory.inttofloat(n) end

---Reinterprets the bits of an 8 byte integer `n` as a double and returns it.
---This does not convert from an int to a double, but reinterprets the memory.
---@nodiscard
---@param n qword
---@return number
function memory.inttodouble(n) end

---Reinterprets the bits of a float `n` as a 4 byte integer and returns it.
---This does not convert from an int to a float, but reinterprets the memory.
---@nodiscard
---@param n number
---@return integer
function memory.floattoint(n) end

---Reinterprets the bits of a 8 byte integer `n` as a double and returns it.
---This does not convert from an int to a float, but reinterprets the memory.
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
---@nodiscard
---@param address integer
---@return integer
function memory.readbytesigned(address) end

---Reads an unsigned byte from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return integer
function memory.readbyte(address) end

---Reads a signed word (2 bytes) from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return integer
function memory.readwordsigned(address) end

---Reads an unsigned word (2 bytes) from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return integer
function memory.readword(address) end

---Reads a signed dword (4 bytes) from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return integer
function memory.readdwordsigned(address) end

---Reads an unsigned dword (4 bytes) from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return integer
function memory.readdword(address) end

---Reads a signed qword (8 bytes) from memory at `address` and returns it as a table of the upper and lower 4 bytes.
---@nodiscard
---@param address integer
---@return qword
function memory.readqwordsigned(address) end

---Reads an unsigned qword (8 bytes) from memory at `address` and returns it as a table of the upper and lower 4 bytes.
---@nodiscard
---@param address integer
---@return integer
function memory.readqword(address) end

---Reads a float (4 bytes) from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return number
function memory.readfloat(address) end

---Reads a double (8 bytes) from memory at `address` and returns it.
---@nodiscard
---@param address integer
---@return number
function memory.readdouble(address) end

---Reads `size` bytes from memory at `address` and returns them.
---The memory is treated as signed if `size` is is negative.
---@nodiscard
---@param address integer
---@param size 1|2|4|8|-1|-2|-4|-8
---@return nil
function memory.readsize(address, size) end

---Writes an unsigned byte to memory at `address`.
---@param address integer
---@param data integer
---@return nil
function memory.writebyte(address, data) end

---Writes an unsigned word (2 bytes) to memory at `address`.
---@param address integer
---@param data integer
---@return nil
function memory.writeword(address, data) end

---Writes an unsigned dword (4 bytes) to memory at `address`.
---@param address integer
---@param data integer
---@return nil
function memory.writedword(address, data) end

---Writes an unsigned qword consisting of a table with the upper and lower 4 bytes to memory at `address`.
---@param address integer
---@param data qword
---@return nil
function memory.writeqword(address, data) end

---Writes a float to memory at `address`.
---@param address integer
---@param data number
---@return nil
function memory.writefloat(address, data) end

---Writes a double to memory at `address`.
---@param address integer
---@param data number
---@return nil
function memory.writedouble(address, data) end

---Writes `size` bytes to memory at `address`.
---The memory is treated as signed if `size` is is negative.
---@param address integer
---@param size 1|2|4|8|-1|-2|-4|-8
---@param data integer|qword
---@return nil
function memory.writesize(address, size, data) end

---See [memory.recompile](lua://memory.recompile).
---@param addr integer
function memory.recompilenow(addr) end

---Queues up a recompilation of the block at the specified address.
---@param addr integer
function memory.recompile(addr) end

---See [memory.recompile](lua://memory.recompile).
---@param addr integer
function memory.recompilenext(addr) end

---Queues up a recompilation of all blocks.
function memory.recompilenextall() end

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

---Clears one or all images.
---@param idx integer The identifier of the image to clear. If it is 0, clear all iamges.
function wgui.deleteimage(idx) end

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


-- d2d functions
--#region

---@alias brush integer

---Creates a brush from a color and returns it. D2D colors range from 0 to 1.
---@param r number
---@param g number
---@param b number
---@param a number
---@return brush
function d2d.create_brush(r, g, b, a) end

---Frees a brush.
---It is a good practice to free all brushes after you are done using them.
---@param brush brush
function d2d.free_brush(brush) end

---Sets clear behavior.
---If this function is never called, the screen will not be cleared.
---If it is called, the screen will be cleared with the specified color.
---@param r number
---@param g number
---@param b number
---@param a number
function d2d.clear(r, g, b, a) end

---Draws a filled in rectangle at the specified coordinates and color.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param brush brush
---@return nil
function d2d.fill_rectangle(x1, y1, x2, y2, brush) end

---Draws the border of a rectangle at the specified coordinates and color.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param thickness number
---@param brush brush
---@return nil
function d2d.draw_rectangle(x1, y1, x2, y2, thickness, brush) end

---Draws a filled in ellipse at the specified coordinates and color.
---@param x integer
---@param y integer
---@param radiusX integer
---@param radiusY integer
---@param brush brush
---@return nil
function d2d.fill_ellipse(x, y, radiusX, radiusY, brush) end

---Draws the border of an ellipse at the specified coordinates and color.
---@param x integer
---@param y integer
---@param radiusX integer
---@param radiusY integer
---@param thickness number
---@param brush brush
---@return nil
function d2d.draw_ellipse(x, y, radiusX, radiusY, thickness, brush) end

---Draws a line from `(x1, y1)` to `(x2, y2)` in the specified color.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param thickness number
---@param brush brush
---@return nil
function d2d.draw_line(x1, y1, x2, y2, thickness, brush) end

---Draws the text `text` at the specified coordinates, color, font, and alignment.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param text string
---@param fontname string
---@param fontsize number
---@param fontweight number
---@param fontstyle 0|1|2|3 0: normal, 1: bold, 2: italic, 3: bold + italic.
---@param horizalign integer
---@param vertalign integer
---@param options integer
---@param brush brush pass 0 if you don't know what you're doing
---@return nil
function d2d.draw_text(x1, y1, x2, y2, text, fontname, fontsize, fontweight,
                       fontstyle, horizalign, vertalign, options, brush)
end

---Returns the width and height of the specified text.
---@param text string
---@param fontname string
---@param fontsize number
---@param max_width number
---@param max_height number
---@return {width: integer, height: integer}
function d2d.get_text_size(text, fontname, fontsize, max_width, max_height) end

---Specifies a rectangle to which all subsequent drawing operations are clipped.
---This clip is put onto a stack.
---It can then be popped off the stack with `wgui.d2d_pop_clip`.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@return nil
function d2d.push_clip(x1, y1, x2, y2) end

---Pops the most recent clip off the clip stack.
---@return nil
function d2d.pop_clip() end

---Draws a filled in rounded rectangle at the specified coordinates, color and radius.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param radiusX number
---@param radiusY number
---@param brush brush
---@return nil
function d2d.fill_rounded_rectangle(x1, y1, x2, y2, radiusX, radiusY, brush) end

---Draws the border of a rounded rectangle at the specified coordinates, color and radius.
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param radiusX number
---@param radiusY number
---@param thickness number
---@param brush brush
---@return nil
function d2d.draw_rounded_rectangle(x1, y1, x2, y2, radiusX, radiusY, thickness,
                                    brush)
end

---Loads an image file from `path` which you can then access through `identifier`.
---@param path string
---@return integer
function d2d.load_image(path) end

---Frees the image at `identifier`.
---@param identifier number
---@return nil
function d2d.free_image(identifier) end

---Draws an image by taking the pixels in the source rectangle of the image, and drawing them to the destination rectangle on the screen.
---@param destx1 integer
---@param desty1 integer
---@param destx2 integer
---@param desty2 integer
---@param srcx1 integer
---@param srcy1 integer
---@param srcx2 integer
---@param srcy2 integer
---@param opacity number
---@param interpolation integer 0: nearest neighbor, 1: linear, -1: don't use.
---@param identifier number
---@return nil
function d2d.draw_image(destx1, desty1, destx2, desty2, srcx1, srcy1, srcx2,
                        srcy2, opacity, interpolation, identifier)
end

---Returns the width and height of the image at `identifier`.
---@nodiscard
---@param identifier number
---@return {width: integer, height: integer}
function d2d.get_image_info(identifier) end

---Sets the text antialiasing mode.
---More info [here](https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ne-d2d1-d2d1_text_antialias_mode).
---@param mode 0|1|2|3|4294967295
function d2d.set_text_antialias_mode(mode) end

---Sets the antialiasing mode.
---More info [here](https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ne-d2d1-d2d1_antialias_mode).
---@param mode 0|1|4294967295
function d2d.set_antialias_mode(mode) end

---Draws to an image and returns its identifier.
---@param width integer
---@param height integer
---@param callback fun()
---@return number
function d2d.draw_to_image(width, height, callback) end

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

---Opens a window where the user can input text.
---If `OK` is clicked, that text is returned.
---If `Cancel` is clicked or the window is closed, `nil` is returned.
---@nodiscard
---@param title string? The title of the text box. Defaults to "input:".
---@param placeholder string? The text box is filled with this string when it opens. Defaults to "".
---@return string|nil
function input.prompt(title, placeholder) end

---Gets the name of a key.
---@nodiscard
---@param key integer
---@return string
function input.get_key_name_text(key) end

--#endregion


-- joypad functions
--#region

---@alias JoypadInputs {
---right: boolean,
---left: boolean,
---down: boolean,
---up: boolean,
---start: boolean,
---Z: boolean,
---B: boolean,
---A: boolean,
---Cright: boolean,
---Cleft: boolean,
---Cdown: boolean,
---Cup: boolean,
---R: boolean,
---L: boolean,
---Y: integer,
---X: integer,
---}


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

---Begins a warp modify.
---@param inputs {[string]: boolean}[]
function movie.begin_warp_modify(inputs) end

--#endregion


-- savestate functions
--#region

---Saves a savestate to `filename`.
---@param filename string
---@return nil
function savestate.savefile(filename) end

---Loads a savestate from `filename`.
---@param filename string
---@return nil
function savestate.loadfile(filename) end

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
---@param buffer ByteBuffer The buffer to use for the operation. Can be empty if the `job` is `save`.
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
---@param type integer Unknown.
---@return string
function iohelper.filediag(filter, type) end

--#endregion


-- avi functions
--#region

---Begins an avi recording using the previously saved encoding settings.
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

---@class Hotkey Represents a combination of keys.
---@field key VKeycodes? The key that is pressed to trigger the hotkey. Note that this is a virtual keycode.
---@field ctrl boolean? Whether the control modifier is pressed.
---@field shift boolean? Whether the shift modifier is pressed.
---@field alt boolean? Whether the alt modifier is pressed.

---Shows a dialog prompting the user to enter a hotkey.
---@param caption string The headline to display in the dialog.
---@return Hotkey|nil The hotkey that was entered, or `nil` if the user cancelled the dialog.
function hotkey.prompt(caption) end

--#endregion


-- action functions
--#region

---@alias ActionFilter string
---An action filter that can be either a fully-qualified or partially-qualified `"Category > Subcategory[] [ > Name ]"`.
---This is usually used to refer to groups of actions, but can also refer to a single action.

---@alias ActionPath string
---A fully-qualified action path in the format `"Category > Subcategory[] > Name"`.
---An action path is a subset of the action filter that is guaranteed to be fully-qualified, meaning it contains all segments of the path.

---@class ActionParams
---@field path ActionPath The action's path.
---@field down_callback fun() The callback to be invoked when the action is initially triggered.
---@field up_callback fun()? The callback to be invoked when the action has been released. Can be null.
---@field get_enabled (fun(): boolean)? The function used to determine whether the action is enabled. If null, the action will be considered enabled.
---@field get_active (fun(): boolean)? The function used to determine whether the action is "active". The active state usually means a checked or toggled UI state. If null, the action will be considered inactive.
---@field get_display_name (fun(): string)? The function used to determine the function's display name. If null, the display name will be derived from the path.

---Adds an action to the action registry. Any action with the same path will be replaced.
---@param params ActionParams The action parameters.
---@return boolean # Whether the operation succeeded.
function action.add(params) end

---Removes actions matching the specified filter.
---@param filter ActionFilter A filter.
---@return boolean # Whether the operation succeeded.
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

---Notifies about the enabled state of actions matching a filter changing.
---@param filter ActionFilter A filter.
function action.notify_enabled_changed(filter) end

---Notifies about the active state of actions matching a filter changing.
---@param filter ActionFilter A filter.
function action.notify_active_changed(filter) end

---Notifies about the display name of actions matching a filter changing.
---@param filter ActionFilter A filter.
function action.notify_display_name_changed(filter) end

---Gets the display name for a given filter.
---@param filter ActionFilter A filter.
---@param ignore_override boolean? Whether to ignore the display name override.
---@return string # The action's display name or an empty string if the display name couldn't be resolved.
function action.get_display_name(filter, ignore_override) end

---Gets all action paths that match the specified filter.
---@param filter ActionFilter? The action path filter. If the path is unqualified, all actions under the last category or subcategory will be returned. If the path is empty, all actions will be returned.
---@return ActionPath[] # The list of action paths that match the filter.
function action.get_actions_matching_filter(filter) end

---Manually invokes an action by its path.
---@param path ActionPath A path.
---@param up boolean? Whether the invocation is considered as "releasing" the action.
function action.invoke(path, up) end

--#endregion