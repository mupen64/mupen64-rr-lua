/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Encoder.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#include <capture/CaptureManager.h>

class FFmpegEncoder : public Encoder
{
  public:
    std::optional<std::wstring> start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t *image) override;
    bool append_audio(uint8_t *audio, size_t length, uint8_t bitrate) override;
    std::wstring get_desired_extension() const override;

  private:
    bool write_av_packet(int stream_index, uint8_t *data, int size, int64_t pts, int64_t duration);

    Params m_params{};

    STARTUPINFO m_si{};
    PROCESS_INFORMATION m_pi{};

    HANDLE m_pipe{};
    HANDLE m_pipe_write_event{};

    AVFormatContext *m_fmt_ctx{};
    AVIOContext *m_avio_ctx{};
    AVStream *m_video_stream{};
    AVStream *m_audio_stream{};

    int64_t m_video_pts = 0;
    int64_t m_audio_pts = 0;

    size_t m_video_frame = 0;
    double_t m_audio_frame = 0.0;
    size_t m_dropped_frames = 0;
    bool m_last_write_was_video = false;

    std::vector<uint8_t> m_silence_buf;
};
