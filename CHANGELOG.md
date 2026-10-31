# 1.5.0-9

## ✨ Features

- Added F3DEX3 microcode support to TASVideo

## 🐞 Bugfixes

- Fixed screenshot overwriting when more than one is taken within a second
- Fixed screenshots not respecting configured screenshot directory

## ⚡ Performance

- Improved Pure Interpreter core performance when breakpoints are present

# 1.5.0-8

This release fixes some annoying bugs and adds new parameters to a Lua API.

## ✨ Features

- Added new `emu.get_pref` preferences ("caret_blink_time", "high_contrast", "reduced_motion")
- Added `dialogs.show_notification`

## 🐞 Bugfixes

- Fixed TASVideo crashing on certain romhacks (e.g. B3313 v0.7)
- Fixed crash when recompiling not-yet-reached blocks (affected `memory.recompile`)
- Fixed dark mode issues
