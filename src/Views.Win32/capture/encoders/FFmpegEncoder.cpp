/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "FFmpegEncoder.h"
#include <DialogService.h>
#include <Config.h>
#include <gui/Main.h>
#include <gui/Loggers.h>

std::wstring FFmpegEncoder::start(Params params)
{
    m_params = params;

    const auto bufsize_video = 2048;
    const auto bufsize_audio = m_params.arate * 8;

    g_view_logger->info("[FFmpegEncoder] arate: {}, bufsize_video: {}, bufsize_audio: {}\n", m_params.arate, bufsize_video, bufsize_audio);

#define VIDEO_PIPE_NAME L"\\\\.\\pipe\\mupenvideo"
#define AUDIO_PIPE_NAME L"\\\\.\\pipe\\mupenaudio"

    m_video_pipe = CreateNamedPipe(
    VIDEO_PIPE_NAME,
    PIPE_ACCESS_OUTBOUND,
    PIPE_TYPE_BYTE | PIPE_WAIT,
    PIPE_UNLIMITED_INSTANCES,
    bufsize_video,
    bufsize_video,
    0,
    nullptr);

    if (!m_video_pipe)
    {
        return L"Failed to create video pipe.";
    }

    m_audio_pipe = CreateNamedPipe(
    AUDIO_PIPE_NAME,
    PIPE_ACCESS_OUTBOUND,
    PIPE_TYPE_BYTE | PIPE_WAIT,
    PIPE_UNLIMITED_INSTANCES,
    bufsize_audio,
    bufsize_audio,
    0,
    nullptr);

    if (!m_audio_pipe)
    {
        CloseHandle(m_video_pipe);
        return L"Failed to create audio pipe.";
    }

    static wchar_t options[4096]{};
    memset(options, 0, sizeof(options));

    wsprintf(options,
             g_config.ffmpeg_final_options.data(),
             m_params.width,
             m_params.height,
             m_params.fps,
             VIDEO_PIPE_NAME,
             m_params.arate,
             AUDIO_PIPE_NAME,
             m_params.path.wstring().data());

    g_view_logger->info(L"[FFmpegEncoder] Starting encode with commandline:");
    g_view_logger->info(L"[FFmpegEncoder] {}", options);

    DeleteFile(params.path.wstring().c_str());

    if (!CreateProcess(g_config.ffmpeg_path.c_str(),
                       options,
                       nullptr,
                       nullptr,
                       FALSE,
                       NULL,
                       nullptr,
                       nullptr,
                       &m_si,
                       &m_pi))
    {

        g_view_logger->info(L"CreateProcess failed ({}).", GetLastError());
        CloseHandle(m_video_pipe);
        CloseHandle(m_audio_pipe);
        return std::format(L"Failed to start ffmpeg process! Does ffmpeg exist on disk at '{}'?", g_config.ffmpeg_path);
    }

    m_silence_buffer = static_cast<uint8_t*>(calloc(params.arate, 1));
    m_blank_buffer = static_cast<uint8_t*>(calloc(m_params.width * m_params.height * 3, 1));
    m_dropped_frames = 0;

    m_video_thread = std::thread(&FFmpegEncoder::write_video_thread, this);
    m_audio_thread = std::thread(&FFmpegEncoder::write_audio_thread, this);

    Sleep(500);

    return L"";
}

bool FFmpegEncoder::stop()
{
    m_stop_thread = true;
    m_video_cv.notify_all();
    m_audio_cv.notify_all();

    // HACK: Give it some time to maybe accept the last writes...
    Sleep(500);

    CancelIo(m_video_pipe);
    CancelIo(m_audio_pipe);
    DisconnectNamedPipe(m_video_pipe);
    DisconnectNamedPipe(m_audio_pipe);

    WaitForSingleObject(m_pi.hProcess, INFINITE);
    CloseHandle(m_pi.hProcess);
    CloseHandle(m_pi.hThread);
    m_audio_thread.join();
    m_video_thread.join();

    if (!this->m_video_queue.empty() || !this->m_audio_queue.empty())
    {
        DialogService::show_dialog(std::format(L"Capture stopped with {} video, {} audio elements remaining in queue!\nThe capture might be corrupted.", this->m_video_queue.size(), this->m_audio_queue.size()).c_str(), L"FFmpeg");
    }

    if (m_dropped_frames > 0)
    {
        DialogService::show_dialog(std::format(L"{} frames were dropped during capture to avoid out-of-memory errors.\nThe capture might contain empty frames.", m_dropped_frames).c_str(), L"FFmpeg");
    }

    free(m_silence_buffer);
    free(m_blank_buffer);
    return true;
}

static bool write_pipe_checked(const HANDLE pipe, const char* buffer, const unsigned buffer_size, const bool is_video)
{
    DWORD written = 0;
    const auto result = WriteFile(pipe, buffer, buffer_size, &written, nullptr);
    if (written != buffer_size || !result)
    {
        // g_view_logger->error("[FFmpegEncoder] Error writing to {} pipe, error code {}", is_video ? "video" : "audio", GetLastError());
        return false;
    }

    return true;
}

bool FFmpegEncoder::append_audio_impl(uint8_t* audio, size_t length)
{
    uint8_t* buf = m_silence_buffer;
    if (audio != m_silence_buffer)
    {
        buf = static_cast<uint8_t*>(malloc(length));
        memcpy(buf, audio, length);
    }

    m_last_write_was_video = false;

    {
        std::lock_guard lock(m_audio_queue_mutex);
        this->m_audio_queue.emplace(buf, length);
    }
    m_audio_cv.notify_one();

    return true;
}

bool FFmpegEncoder::append_video(uint8_t* image)
{
    if (g_config.synchronization_mode == 1)
    {
        if (m_last_write_was_video)
        {
            return true;
        }
    }
    else if (g_config.synchronization_mode == 2)
    {
        if (core_vr_get_lag_count() > 2)
        {
            const auto samples_per_frame = static_cast<double>(m_params.arate) / 64;
            append_audio_impl(m_silence_buffer, static_cast<size_t>(round(samples_per_frame)));
        }
    }

    m_last_write_was_video = true;

    auto buf = static_cast<uint8_t*>(malloc(m_params.width * m_params.height * 3));

    // HACK: If we run out of memory, we start writing empty video frames
    // This can happen when encoding at high resolutions, as ffmpeg might not start processing the pipe writes in time
    if (!buf)
    {
        g_view_logger->error("[FFmpegEncoder] Out of memory, writing blank video frame");
        ++m_dropped_frames;

        {
            std::lock_guard lock(m_video_queue_mutex);
            m_video_queue.emplace(m_blank_buffer, false);
        }
        m_video_cv.notify_one();
        return true;
    }

    memcpy(buf, image, m_params.width * m_params.height * 3);

    {
        std::lock_guard lock(m_video_queue_mutex);
        m_video_queue.emplace(buf, true);
    }
    m_video_cv.notify_one();

    return true;
}

bool FFmpegEncoder::append_audio(uint8_t* audio, size_t length, uint8_t)
{
    auto buf = static_cast<uint8_t*>(malloc(length));
    memcpy(buf, audio, length);

    return append_audio_impl(buf, length);
}

void FFmpegEncoder::write_audio_thread()
{
    g_view_logger->trace("[FFmpegEncoder] Audio thread ready");

    while (!this->m_stop_thread)
    {
        std::unique_lock lock(m_audio_queue_mutex);
        m_audio_cv.wait(lock, [this] { return !m_audio_queue.empty() || m_stop_thread; });

        if (this->m_audio_queue.empty())
            continue;

        auto tuple = this->m_audio_queue.front();
        this->m_audio_queue.pop();
        lock.unlock();

        const auto buf = std::get<0>(tuple);
        const auto len = std::get<1>(tuple);

        write_pipe_checked(m_audio_pipe, (char*)buf, len, false);
        if (buf != m_silence_buffer)
        {
            free(buf);
        }
    }
}

void FFmpegEncoder::write_video_thread()
{
    g_view_logger->trace("[FFmpegEncoder] Video thread ready");

    while (!this->m_stop_thread)
    {
        std::unique_lock lock(m_video_queue_mutex);
        m_video_cv.wait(lock, [this] { return !m_video_queue.empty() || m_stop_thread; });

        if (m_video_queue.empty())
            continue;

        auto pair = this->m_video_queue.front();
        this->m_video_queue.pop();
        lock.unlock();

        write_pipe_checked(m_video_pipe, (char*)pair.first, m_params.width * m_params.height * 3, true);
        if (pair.second)
        {
            free(pair.first);
        }
    }
}
