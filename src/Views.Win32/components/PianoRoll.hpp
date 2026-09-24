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

const std::string BASE = APP + "Piano Roll > ";
const std::string COPY = BASE + "Edit > Copy";
const std::string PASTE = BASE + "Edit > Paste ---";
const std::string UNDO = BASE + "Edit > Undo";
const std::string REDO = BASE + "Edit > Redo ---";
const std::string INSERT_FRAME = BASE + "Edit > Insert Frame";
const std::string CLEAR = BASE + "Edit > Clear";
const std::string DELETE = BASE + "Edit > Delete";

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
