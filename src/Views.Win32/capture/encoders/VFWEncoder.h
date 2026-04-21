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

  private:
    static constexpr auto SOUND_BUF_SIZE = 44100 * 2 * 2;
    static constexpr auto MAX_AVI_SIZE = 0x7B9ACA00;
    static constexpr auto RESAMPLED_FREQ = 44100;
    static constexpr size_t MAX_PENDING_WORK_ITEMS = 128;
    static constexpr size_t MAX_PENDING_WORK_BYTES = 64 * 1024 * 1024;

    bool write_sound(uint8_t *buf, int len, int min_write_size, int max_write_size, BOOL force, uint8_t bitrate);
    bool append_video_impl(uint8_t *image);
    bool save_options() const;
    bool load_options();
    bool stop_impl(bool fail_stop = true);

    enum class WorkType : std::uint8_t
    {
        Video,
        Audio,
    };

    struct WorkItem
    {
        WorkType type{};
        std::vector<uint8_t> data;
        uint8_t bitrate = 16;
        size_t frame_count = 1;
        bool force = false;
        double_t desync = 0.0;
    };

    void worker_loop();
    bool enqueue_work(WorkItem item);
    void wait_for_all_work();

    Params m_params{};
    AVICOMPRESSOPTIONS m_avi_options{};

    bool m_splitting = false;
    size_t m_splits = 0;

    size_t m_frame = 0;
    size_t m_sample = 0;
    size_t m_video_frame = 0;
    double_t m_audio_frame = 0;
    uint8_t m_sound_buf[SOUND_BUF_SIZE]{};
    uint8_t m_sound_buf_empty[SOUND_BUF_SIZE]{};
    int sound_buf_pos = 0;
    long last_sound = 0;

    BITMAPINFOHEADER m_info_hdr{};
    PAVIFILE m_avi_file{};

    AVISTREAMINFO m_video_stream_hdr{};
    PAVISTREAM m_video_stream{};
    PAVISTREAM m_compressed_video_stream{};

    AVISTREAMINFO m_sound_stream_hdr{};
    PAVISTREAM m_sound_stream{};
    WAVEFORMATEX m_sound_format{};

    size_t m_avi_file_size = 0;

    std::mutex m_work_mutex;
    std::condition_variable m_work_cv{};
    std::condition_variable m_work_drained_cv{};
    std::deque<WorkItem> m_work_queue;
    size_t m_pending_work_bytes = 0;
    size_t m_work_in_flight = 0;
    bool m_worker_running = false;
    bool m_worker_stop_requested = false;
    bool m_worker_failed = false;
};
