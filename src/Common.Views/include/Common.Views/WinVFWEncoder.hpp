/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/Encoder.hpp>
#include <Common.Views/Resampler.hpp>
#include <windows.h>
#include <vfw.h>

class WinVFWEncoder final : public Encoder
{
  public:
    std::optional<std::string> start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t *image) override;
    bool append_audio(uint8_t *audio, size_t length, uint8_t bitrate, uint32_t source_rate) override;
    std::string get_desired_extension() const override;

  private:
    static constexpr auto max_avi_size = 0x7B9ACA00;
    static constexpr size_t max_pending_work_items = 128;
    static constexpr size_t max_pending_work_bytes = 64 * 1024 * 1024;

    bool append_video_impl(uint8_t *image);
    bool save_options() const;
    bool load_options();
    bool stop_impl(bool fail_stop = true);

    Params m_params{};
    Resampler m_resampler;
    AVICOMPRESSOPTIONS m_avi_options{};

    bool m_splitting = false;
    size_t m_splits = 0;

    size_t m_frame = 0;
    size_t m_sample = 0;
    size_t m_video_frame = 0;
    double m_audio_frame = 0;
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
