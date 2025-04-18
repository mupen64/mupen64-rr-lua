/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

int rsmp_resample(short** dst, int dst_freq, const short* src, int src_freq, int src_bitrate, int src_len);


int rsmp_get_resample_len(int dst_freq, int src_freq, int src_bitrate, int src_len);
