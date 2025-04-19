/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * A module responsible for implementing audio resampling.
 */
namespace Resampler
{
    /**
     * \brief Resamples audio data from one frequency to another.
     * \param dst Pointer to the destination buffer. This buffer is statically allocated and doesn't need to be freed by the caller.
     * \param dst_freq The destination frequency.
     * \param src Pointer to the source buffer.
     * \param src_freq The source frequency.
     * \param src_bitrate The source bitrate.
     * \param src_len The length of the source buffer in bytes.
     * \return The amount of bytes written to the destination buffer.
     * \remark This function is not thread-safe.
     */
    int resample(short** dst, int dst_freq, const short* src, int src_freq, int src_bitrate, int src_len);

    /**
     * \brief Calculates the length of the destination buffer based on the source buffer's length and the desired destination frequency.
     * \param dst_freq The destination frequency.
     * \param src_freq The source frequency.
     * \param src_bitrate The source bitrate.
     * \param src_len The length of the source buffer in bytes.
     * \return The length of the destination buffer in bytes.
     */
    int get_resample_len(int dst_freq, int src_freq, int src_bitrate, int src_len);
} // namespace Resampler
