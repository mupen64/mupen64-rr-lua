/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Disassembles an instruction buffer to a file
 * \param f File to write the disassembly to
 * \param t The instruction buffer
 */
void disasm(FILE *f, unsigned long t[0x1000 / 4]);
