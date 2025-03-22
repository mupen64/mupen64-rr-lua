/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void cht_execute();

/**
 * \brief Pushes the specified cheat collection layer onto the execution stack.
 * This overrides the current execution list with the one provided until the cht_layer_pop function is called; the topmost layer is always used.
 */
void cht_layer_push(const std::vector<core_cheat>&);

/**
 * \brief Pops the current cheat collection layer from the execution stack. If the stack is empty, the host-provided cheat collection is used.
 */
void cht_layer_pop();

/**
 * \brief Reads the cheats specified in a file.
 * \param path The cheat collection file to read from.
 * \param cheats The cheat vector to write the cheats into.
 * \return Whether the operation succeeded.
 */
bool cht_read_from_file(const std::filesystem::path& path, std::vector<core_cheat>& cheats);

/**
 * \brief Serializes the host-provided cheat collection to a string which can be understood by cht_read_from_file. If the collection is empty, an empty string is returned.
 */
std::wstring cht_serialize();
