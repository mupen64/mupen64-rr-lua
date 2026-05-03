/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "FFmpegEncoder.h"
#include <DialogService.h>
#include <Config.h>

#define NUT_PIPE_NAME L"\\\\.\\pipe\\mupennut"

struct PipeIO
{
    HANDLE pipe;
    HANDLE event;
};

static int avio_write_cb(void *opaque, const uint8_t *buf, int size)
{
    auto *io = static_cast<PipeIO *>(opaque);
    OVERLAPPED ov{};
    ov.hEvent = io->event;
    ResetEvent(ov.hEvent);

    DWORD written = 0;
    if (!WriteFile(io->pipe, buf, size, nullptr, &ov))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            g_view_logger->error("[FFmpegEncoder] WriteFile failed: {}", GetLastError());
            return AVERROR(EIO);
        }
    }
    if (!GetOverlappedResult(io->pipe, &ov, &written, TRUE))
    {
        g_view_logger->error("[FFmpegEncoder] GetOverlappedResult failed: {}", GetLastError());
        return AVERROR(EIO);
    }
    return static_cast<int>(written);
}

static int64_t avio_seek_cb(void *, int64_t, int)
{
    return AVERROR(ENOSYS);
}

std::optional<std::wstring> FFmpegEncoder::start(Params params)
{
    m_params = params;
    m_video_pts = 0;
    m_audio_pts = 0;
    m_dropped_frames = 0;
    m_last_write_was_video = false;

    m_pipe = CreateNamedPipe(NUT_PIPE_NAME, PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_WAIT, 1,
                             1 << 20, // 1 MB write buffer
                             0, 0, nullptr);

    if (m_pipe == INVALID_HANDLE_VALUE)
    {
        return L"Failed to create NUT pipe.";
    }

    m_pipe_write_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    static wchar_t options[4096]{};
    memset(options, 0, sizeof(options));
    wsprintf(options, g_config.ffmpeg_final_options.data(), NUT_PIPE_NAME, m_params.path.wstring().data());

    g_view_logger->info(L"[FFmpegEncoder] Starting encode with commandline:");
    g_view_logger->info(L"[FFmpegEncoder] {}", options);

    DeleteFile(params.path.wstring().c_str());

    memset(&m_si, 0, sizeof(m_si));
    memset(&m_pi, 0, sizeof(m_pi));

    if (!CreateProcess(g_config.ffmpeg_path.c_str(), options, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &m_si,
                       &m_pi))
    {
        g_view_logger->error(L"[FFmpegEncoder] CreateProcess failed ({}).", GetLastError());
        CloseHandle(m_pipe);
        CloseHandle(m_pipe_write_event);
        return std::format(L"Failed to start ffmpeg process! Does ffmpeg exist on disk at '{}'?", g_config.ffmpeg_path);
    }

    {
        OVERLAPPED ov_connect{};
        ov_connect.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        ConnectNamedPipe(m_pipe, &ov_connect);

        HANDLE wait_handles[] = {ov_connect.hEvent, m_pi.hProcess};
        DWORD wait = WaitForMultipleObjects(2, wait_handles, FALSE, 10'000);

        CloseHandle(ov_connect.hEvent);

        if (wait != WAIT_OBJECT_0)
        {
            g_view_logger->error("[FFmpegEncoder] Timed out waiting for FFmpeg to open the pipe.");
            TerminateProcess(m_pi.hProcess, 1);
            CloseHandle(m_pi.hProcess);
            CloseHandle(m_pi.hThread);
            CloseHandle(m_pipe);
            CloseHandle(m_pipe_write_event);
            return L"FFmpeg did not open the input pipe in time.";
        }
    }

    int ret = avformat_alloc_output_context2(&m_fmt_ctx, nullptr, "nut", nullptr);
    if (ret < 0 || !m_fmt_ctx)
    {
        return L"Failed to allocate NUT output context.";
    }

    m_video_stream = avformat_new_stream(m_fmt_ctx, nullptr);
    m_video_stream->id = 0;
    m_video_stream->time_base = {1, static_cast<int>(m_params.fps)};
    m_video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    m_video_stream->codecpar->codec_id = AV_CODEC_ID_RAWVIDEO;
    m_video_stream->codecpar->width = static_cast<int>(m_params.width);
    m_video_stream->codecpar->height = static_cast<int>(m_params.height);
    m_video_stream->codecpar->format = AV_PIX_FMT_BGRA;
    m_video_stream->codecpar->bits_per_coded_sample = 32;
    m_video_stream->codecpar->codec_tag = avcodec_pix_fmt_to_codec_tag(AV_PIX_FMT_BGRA);

    m_audio_stream = avformat_new_stream(m_fmt_ctx, nullptr);
    m_audio_stream->id = 1;
    m_audio_stream->time_base = {1, static_cast<int>(m_params.arate)};
    m_audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    m_audio_stream->codecpar->codec_id = AV_CODEC_ID_PCM_S16LE;
    m_audio_stream->codecpar->sample_rate = static_cast<int>(m_params.arate);
    m_audio_stream->codecpar->format = AV_SAMPLE_FMT_S16;
    m_audio_stream->codecpar->bits_per_coded_sample = 16;
    AVChannelLayout stereo_layout = AV_CHANNEL_LAYOUT_STEREO;
    av_channel_layout_copy(&m_audio_stream->codecpar->ch_layout, &stereo_layout);

    static PipeIO pipe_io{};
    pipe_io.pipe = m_pipe;
    pipe_io.event = m_pipe_write_event;

    constexpr int avio_buf_size = 64 * 1024;
    uint8_t *avio_buf = static_cast<uint8_t *>(av_malloc(avio_buf_size));
    m_avio_ctx =
        avio_alloc_context(avio_buf, avio_buf_size, 1 /*write*/, &pipe_io, nullptr, avio_write_cb, avio_seek_cb);
    m_avio_ctx->seekable = 0;
    m_fmt_ctx->pb = m_avio_ctx;
    m_fmt_ctx->flags |= AVFMT_FLAG_NOBUFFER;

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "strict", "experimental", 0);
    ret = avformat_write_header(m_fmt_ctx, &opts);
    av_dict_free(&opts);

    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        g_view_logger->error("[FFmpegEncoder] avformat_write_header failed: {}", errbuf);
        avformat_free_context(m_fmt_ctx);
        m_fmt_ctx = nullptr;
        CloseHandle(m_pipe);
        CloseHandle(m_pipe_write_event);
        return std::format(L"Failed to write NUT header: {}", std::wstring(errbuf, errbuf + strlen(errbuf)));
    }

    g_view_logger->info("[FFmpegEncoder] NUT stream started ({}x{} @ {} fps, {} Hz audio)", m_params.width,
                        m_params.height, m_params.fps, m_params.arate);

    const auto silence_samples = static_cast<size_t>(round(static_cast<double>(m_params.arate) / 64));
    m_silence_buf.assign(silence_samples * 4, 0);

    return std::nullopt;
}

bool FFmpegEncoder::stop()
{
    if (m_fmt_ctx)
    {
        av_write_trailer(m_fmt_ctx);
        avio_flush(m_avio_ctx);
    }

    DisconnectNamedPipe(m_pipe);
    CloseHandle(m_pipe);
    CloseHandle(m_pipe_write_event);
    m_pipe = nullptr;
    m_pipe_write_event = nullptr;

    WaitForSingleObject(m_pi.hProcess, INFINITE);
    CloseHandle(m_pi.hProcess);
    CloseHandle(m_pi.hThread);

    if (m_avio_ctx)
    {
        av_free(m_avio_ctx->buffer);
        avio_context_free(&m_avio_ctx);
        m_avio_ctx = nullptr;
    }
    if (m_fmt_ctx)
    {
        avformat_free_context(m_fmt_ctx);
        m_fmt_ctx = nullptr;
    }

    m_silence_buf.clear();

    if (m_dropped_frames > 0)
    {
        DialogService::show_dialog(std::format(L"{} frames were dropped during capture due to low memory.\n"
                                               L"The capture might contain empty frames.",
                                               m_dropped_frames)
                                       .c_str(),
                                   L"FFmpeg");
    }

    return true;
}

bool FFmpegEncoder::write_av_packet(int stream_index, uint8_t *data, int size, int64_t pts, int64_t duration)
{
    AVPacket *pkt = av_packet_alloc();
    pkt->data = data;
    pkt->size = size;
    pkt->stream_index = stream_index;
    pkt->pts = pts;
    pkt->dts = pts;
    pkt->duration = duration;

    int ret = av_write_frame(m_fmt_ctx, pkt);

    // Prevent av_packet_free from freeing externally-owned data
    pkt->data = nullptr;
    pkt->size = 0;
    av_packet_free(&pkt);

    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        g_view_logger->error("[FFmpegEncoder] av_write_frame failed: {}", errbuf);
        return false;
    }
    return true;
}

bool FFmpegEncoder::append_video(uint8_t *image)
{
    if (g_config.synchronization_mode == 1)
    {
        if (m_last_write_was_video) return true;
    }
    else if (g_config.synchronization_mode == 2)
    {
        if (g_main_ctx.core_ctx->vr_get_lag_count() > 2)
        {
            append_audio(m_silence_buf.data(), m_silence_buf.size(), 0);
        }
    }

    m_last_write_was_video = true;

    const auto frame_bytes = m_params.width * m_params.height * 4;

    if (!image)
    {
        g_view_logger->error("[FFmpegEncoder] Null image, writing blank video frame");
        ++m_dropped_frames;
        auto *blank = static_cast<uint8_t *>(calloc(frame_bytes, 1));
        const int64_t blank_pts =
            av_rescale_q(m_video_pts++, {1, static_cast<int>(m_params.fps)}, m_video_stream->time_base);
        write_av_packet(m_video_stream->index, blank, static_cast<int>(frame_bytes), blank_pts,
                        av_rescale_q(1, {1, static_cast<int>(m_params.fps)}, m_video_stream->time_base));
        free(blank);
        return true;
    }

    const int64_t pts = av_rescale_q(m_video_pts++, {1, static_cast<int>(m_params.fps)}, m_video_stream->time_base);
    write_av_packet(m_video_stream->index, image, static_cast<int>(frame_bytes), pts,
                    av_rescale_q(1, {1, static_cast<int>(m_params.fps)}, m_video_stream->time_base));
    return true;
}

bool FFmpegEncoder::append_audio(uint8_t *audio, size_t length, uint8_t)
{
    const auto nb_samples = static_cast<int64_t>(length / 4);
    const int64_t pts = av_rescale_q(m_audio_pts, {1, static_cast<int>(m_params.arate)}, m_audio_stream->time_base);
    const int64_t dur = av_rescale_q(nb_samples, {1, static_cast<int>(m_params.arate)}, m_audio_stream->time_base);
    write_av_packet(m_audio_stream->index, audio, static_cast<int>(length), pts, dur);
    m_audio_pts += nb_samples;
    m_last_write_was_video = false;
    return true;
}
