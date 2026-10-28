/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <action/AppActions.hpp>

/**
 * \brief A module responsible for implementing the piano roll frontend.
 */
namespace PianoRoll
{
using namespace AppActions;
#undef DELETE

const std::string BASE = APP + ".piano-roll";
const std::string COPY = BASE + ".edit.copy";
const std::string PASTE = BASE + ".edit.paste";
const std::string UNDO = BASE + ".edit.undo";
const std::string REDO = BASE + ".edit.redo";
const std::string INSERT_FRAME = BASE + ".edit.insert-frame";
const std::string CLEAR = BASE + ".edit.clear";
const std::string DELETE = BASE + ".edit.delete";

/**
 * \brief Initializes the subsystem.
 */
void init();

/**
 * Shows the piano roll window.
 */
void show();

/**
 * \brief Gets the HWND of the piano roll window. Might be invalid.
 */
HWND hwnd();
} // namespace PianoRoll
