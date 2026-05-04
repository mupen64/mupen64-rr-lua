/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Encoder.h"

#include <Vfw.h>

class VFWEncoder final : public Encoder
{
  public:
    std::optional<std::wstring> start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t *image) override;
    bool append_audio(uint8_t *audio, size_t length, uint8_t bitrate) override;
    std::wstring get_desired_extension() const override;

  private:
    static constexpr auto SOUND_BUF_SIZE = 44100 * 2 * 2;
    static constexpr auto MAX_AVI_SIZE = 0x7B9ACA00;
    static constexpr auto RESAMPLED_FREQ = 44100;
    static constexpr size_t MAX_PENDING_WORK_ITEMS = 128;
    static constexpr size_t MAX_PENDING_WORK_BYTES = 64 * 1024 * 1024;

    bool write_sound(uint8_t *buf, int len, uint8_t bitrate);
    bool append_video_impl(uint8_t *image);
    bool save_options() const;
    bool load_options();
    bool stop_impl(bool fail_stop = true);

    Params m_params{};
    AVICOMPRESSOPTIONS m_avi_options{};

    bool m_splitting = false;
    size_t m_splits = 0;

    size_t m_frame = 0;
    size_t m_in_sample = 0;
    size_t m_sample = 0;
    size_t m_video_frame = 0;
    double_t m_audio_frame = 0;
    uint8_t m_sound_buf[SOUND_BUF_SIZE]{};
    uint8_t m_sound_buf_empty[SOUND_BUF_SIZE]{};
    short *m_resampled_sound{};
    int sound_buf_pos = 0;
    uint32_t m_last_sound = 0;
    size_t m_audio_samples = 0;

    BITMAPINFOHEADER m_info_hdr{};
    PAVIFILE m_avi_file{};

    AVISTREAMINFO m_video_stream_hdr{};
    PAVISTREAM m_video_stream{};
    PAVISTREAM m_compressed_video_stream{};

    AVISTREAMINFO m_sound_stream_hdr{};
    PAVISTREAM m_sound_stream{};
    WAVEFORMATEX m_sound_format{};

    size_t m_avi_file_size = 0;
};
