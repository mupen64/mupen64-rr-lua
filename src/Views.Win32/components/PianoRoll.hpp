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

const std::wstring BASE = APP + L"Piano Roll > ";
const std::wstring COPY = BASE + L"Edit > Copy";
const std::wstring PASTE = BASE + L"Edit > Paste ---";
const std::wstring UNDO = BASE + L"Edit > Undo";
const std::wstring REDO = BASE + L"Edit > Redo ---";
const std::wstring INSERT_FRAME = BASE + L"Edit > Insert Frame";
const std::wstring CLEAR = BASE + L"Edit > Clear";
const std::wstring DELETE = BASE + L"Edit > Delete";

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
