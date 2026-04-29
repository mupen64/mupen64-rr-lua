/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <DialogService.h>

#include <capture/CaptureManager.h>
#include <capture/Resampler.h>
#include <capture/encoders/VFWEncoder.h>

// #define VFW_ENCODER_PARALLELIZED

std::optional<std::wstring> VFWEncoder::start(Params params)
{
    if (!m_splitting)
    {
        m_params = params;
    }
    m_avi_file_size = 0;
    m_frame = 0;
    m_info_hdr.biSize = sizeof(BITMAPINFOHEADER);
    m_info_hdr.biWidth = params.width;
    m_info_hdr.biHeight = params.height;
    m_info_hdr.biPlanes = 1;
    m_info_hdr.biBitCount = 32;
    m_info_hdr.biCompression = BI_RGB;
    m_info_hdr.biSizeImage = params.width * params.height * 4;
    m_info_hdr.biXPelsPerMeter = 0;
    m_info_hdr.biYPelsPerMeter = 0;
    m_info_hdr.biClrUsed = 0;
    m_info_hdr.biClrImportant = 0;

    DeleteFile(params.path.wstring().c_str());

    AVIFileInit();
    if (AVIFileOpen(&m_avi_file, params.path.wstring().c_str(), OF_WRITE | OF_CREATE, NULL))
    {
        stop_impl();
        return L"Failed to open output file.";
    }

    ZeroMemory(&m_video_stream_hdr, sizeof(AVISTREAMINFO));
    m_video_stream_hdr.fccType = streamtypeVIDEO;
    m_video_stream_hdr.dwScale = 1;
    m_video_stream_hdr.dwRate = params.fps;
    m_video_stream_hdr.dwSuggestedBufferSize = 0;
    if (AVIFileCreateStream(m_avi_file, &m_video_stream, &m_video_stream_hdr))
    {
        stop_impl();
        return L"Failed to create video file stream.";
    }

    // NOTE: AVIFileCreateStream seems to change the cwd for some reason...
    set_cwd();

    if (params.ask_for_capture_settings && !m_splitting)
    {
        LPAVICOMPRESSOPTIONS avi_options[1] = {&m_avi_options};
        if (!AVISaveOptions(g_main_ctx.hwnd, 0, 1, &m_video_stream, avi_options))
        {
            stop_impl();
            return L"";
        }

        if (!save_options())
        {
            stop_impl();
            return L"Failed to save options.";
        }
    }
    else
    {
        if (!load_options())
        {
            return L"Failed to load options. Verify that the capture preset file is present.";
        }
    }

    if (AVIMakeCompressedStream(&m_compressed_video_stream, m_video_stream, &m_avi_options, NULL) != AVIERR_OK)
    {
        stop_impl();
        return L"Failed to make video compressed stream.";
    }

    if (AVIStreamSetFormat(m_compressed_video_stream, 0, &m_info_hdr,
                           m_info_hdr.biSize + m_info_hdr.biClrUsed * sizeof(RGBQUAD)) != AVIERR_OK)
    {
        stop_impl();
        return L"Failed to set video stream format.";
    }

    m_sample = 0;
    m_sound_format.wFormatTag = WAVE_FORMAT_PCM;
    m_sound_format.nChannels = 2;
    m_sound_format.nSamplesPerSec = 44100;
    m_sound_format.nAvgBytesPerSec = 44100 * (2 * 16 / 8);
    m_sound_format.nBlockAlign = 2 * 16 / 8;
    m_sound_format.wBitsPerSample = 16;
    m_sound_format.cbSize = 0;

    ZeroMemory(&m_sound_stream_hdr, sizeof(AVISTREAMINFO));
    m_sound_stream_hdr.fccType = streamtypeAUDIO;
    m_sound_stream_hdr.dwQuality = (DWORD)-1;
    m_sound_stream_hdr.dwScale = m_sound_format.nBlockAlign;
    m_sound_stream_hdr.dwInitialFrames = 1;
    m_sound_stream_hdr.dwRate = m_sound_format.nAvgBytesPerSec;
    m_sound_stream_hdr.dwSampleSize = m_sound_format.nBlockAlign;
    if (AVIFileCreateStream(m_avi_file, &m_sound_stream, &m_sound_stream_hdr))
    {
        stop_impl();
        return L"Failed to create audio stream.";
    }
    if (AVIStreamSetFormat(m_sound_stream, 0, &m_sound_format, sizeof(WAVEFORMATEX)) != AVIERR_OK)
    {
        stop_impl();
        return L"Failed to set audio stream format.";
    }

    memset(m_sound_buf_empty, 0, sizeof(m_sound_buf_empty));
    memset(m_sound_buf, 0, sizeof(m_sound_buf));
    last_sound = 0;

#ifdef VFW_ENCODER_PARALLELIZED
    {
        std::scoped_lock lock(m_work_mutex);
        m_work_queue.clear();
        m_pending_work_bytes = 0;
        m_work_in_flight = 0;
        m_worker_stop_requested = false;
        m_worker_failed = false;
        m_worker_running = true;
    }

    std::thread(&VFWEncoder::worker_loop, this).detach();
#endif

    return std::nullopt;
}

bool VFWEncoder::stop_impl(const bool fail_stop)
{
#ifdef VFW_ENCODER_PARALLELIZED
    wait_for_all_work();

    if (!m_worker_failed)
    {
        WorkItem flush{};
        flush.type = WorkType::Audio;
        flush.force = true;
        flush.bitrate = 16;
        flush.desync = static_cast<double_t>(m_video_frame) - m_audio_frame;
        if (!enqueue_work(std::move(flush)))
        {
            m_worker_failed = true;
        }
    }

    {
        std::unique_lock lock(m_work_mutex);
        m_worker_stop_requested = true;
        m_work_cv.notify_all();
        m_work_drained_cv.wait(lock, [this]() { return !m_worker_running; });
    }
#endif

    if (m_compressed_video_stream)
    {
        AVIStreamClose(m_compressed_video_stream);
        m_compressed_video_stream = nullptr;
    }
    if (m_video_stream)
    {
        AVIStreamRelease(m_video_stream);
        m_video_stream = nullptr;
    }
    if (m_sound_stream)
    {
        AVIStreamClose(m_sound_stream);
        m_sound_stream = nullptr;
    }
    if (m_avi_file)
    {
        AVIFileClose(m_avi_file);
        m_avi_file = nullptr;
    }
    AVIFileExit();

    if (fail_stop)
    {
        DeleteFile(m_params.path.wstring().c_str());
    }

    return !m_worker_failed;
}

bool VFWEncoder::stop()
{
    return this->stop_impl(false);
}

bool VFWEncoder::append_video(uint8_t *image)
{
    if (g_config.synchronization_mode != static_cast<int>(CaptureManager::Sync::Audio) &&
        g_config.synchronization_mode != static_cast<int>(CaptureManager::Sync::None))
    {
        return true;
    }

#ifdef VFW_ENCODER_PARALLELIZED
    if (m_worker_failed)
    {
        return false;
    }

    size_t frame_count = 1;

    // AUDIO SYNC
    // This type of syncing assumes the audio is authoratative, and drops or duplicates frames to keep the video as
    // close to it as possible. Some games stop updating the screen entirely at certain points, such as loading zones,
    // which will cause audio to drift away by default. This method of syncing prevents this, at the cost of the video
    // feed possibly freezing or jumping (though in practice this rarely happens - usually a loading scene just appears
    // shorter or something).

    if (g_config.synchronization_mode == (int)CaptureManager::Sync::Audio)
    {
        frame_count = 0;
        while (true)
        {
            const int overshot = (int)(m_audio_frame - (double)m_video_frame + 0.2);
            if (overshot == 0) break;

            RT_ASSERT(overshot >= 0, L"Video is ahead of audio");

            ++frame_count;
            ++m_video_frame;
        }

        if (frame_count == 0)
        {
            return true;
        }
    }
    else
    {
        ++m_video_frame;
    }

    WorkItem item{};
    item.type = WorkType::Video;
    item.frame_count = frame_count;
    item.data.resize(m_info_hdr.biSizeImage);
    memcpy(item.data.data(), image, m_info_hdr.biSizeImage);

    return enqueue_work(std::move(item));
#else
    bool result = true;
    if (g_config.synchronization_mode == (int)CaptureManager::Sync::Audio)
    {
        while (true)
        {
            const int overshot = (int)(m_audio_frame - (double)m_video_frame + 0.2);
            if (overshot == 0) break;

            RT_ASSERT(overshot >= 0, L"Video is ahead of audio");

            result = append_video_impl(image);
            m_video_frame++;
        }
    }
    else
    {
        result = append_video_impl(image);
        m_video_frame++;
    }

    return result;

#endif
}

bool VFWEncoder::append_audio(uint8_t *audio, size_t length, uint8_t bitrate)
{
#ifdef VFW_ENCODER_PARALLELIZED

    if (m_worker_failed)
    {
        return false;
    }

    const double_t desync = static_cast<double_t>(m_video_frame) - m_audio_frame;

    if (length > 0)
    {
        m_audio_frame +=
            ((length / 4) / (long double)m_params.arate) *
            g_main_ctx.core_ctx->vr_get_vis_per_second(g_main_ctx.core_ctx->vr_get_rom_header()->Country_code);
    }

    if (g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::Video) && desync > 1.0)
    {
        const long double vis_per_second =
            g_main_ctx.core_ctx->vr_get_vis_per_second(g_main_ctx.core_ctx->vr_get_rom_header()->Country_code);
        int len3 = (int)(m_params.arate / vis_per_second) * (int)desync;
        len3 <<= 2;
        m_audio_frame += ((len3 / 4) / (long double)m_params.arate) * vis_per_second;
    }

    WorkItem item{};
    item.type = WorkType::Audio;
    item.bitrate = bitrate;
    item.force = false;
    item.desync = desync;
    item.data.resize(length);
    if (length > 0)
    {
        memcpy(item.data.data(), audio, length);
    }

    return enqueue_work(std::move(item));
#else
    const int write_size = m_params.arate * 2;

    if (g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::Video) ||
        g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::None))
    {
        // VIDEO SYNC
        // This is the original syncing code, which adds silence to the audio track to get it to line up with video.
        // The N64 appears to have the ability to arbitrarily disable its sound processing facilities and no audio
        // samples are generated. When this happens, the video track will drift away from the audio. This can happen at
        // load boundaries in some games, for example.
        //
        // The only new difference here is that the desync flag is checked for being greater than 1.0 instead of 0.
        // This is because the audio and video in mupen tend to always be diverged just a little bit, but stay in sync
        // over time. Checking if desync is not 0 causes the audio stream to to get thrashed which results in clicks
        // and pops.

        double_t desync = m_video_frame - m_audio_frame;

        if (g_config.synchronization_mode == (int)CaptureManager::Sync::None) // HACK
            desync = 0.0;

        if (desync > 1.0)
        {
            g_view_logger->info("[CaptureManager]: Correcting for A/V desynchronization of %+Lf frames\n", desync);
            int len3 = (int)(m_params.arate / (long double)g_main_ctx.core_ctx->vr_get_vis_per_second(
                                                  g_main_ctx.core_ctx->vr_get_rom_header()->Country_code)) *
                       (int)desync;
            len3 <<= 2;
            const int empty_size = len3 > write_size ? write_size : len3;

            for (int i = 0; i < empty_size; i += 4) *reinterpret_cast<long *>(m_sound_buf_empty + i) = last_sound;

            while (len3 > write_size)
            {
                write_sound(m_sound_buf_empty, write_size, m_params.arate, write_size, FALSE, bitrate);
                len3 -= write_size;
            }
            write_sound(m_sound_buf_empty, len3, m_params.arate, write_size, FALSE, bitrate);
        }
        else if (desync <= -10.0)
        {
            g_view_logger->info("[CaptureManager]: Waiting from A/V desynchronization of %+Lf frames\n", desync);
        }
    }

    write_sound(audio, length, m_params.arate, write_size, FALSE, bitrate);
    last_sound = *(reinterpret_cast<long *>(audio + length) - 1);

    return true;
#endif
}

bool VFWEncoder::write_sound(uint8_t *buf, int len, const int min_write_size, const int max_write_size,
                             const BOOL force, uint8_t bitrate)
{
    if ((len <= 0 && !force) || len > max_write_size) return false;

    if (sound_buf_pos + len > min_write_size || force)
    {
        int len2 = Resampler::get_resample_len(RESAMPLED_FREQ, m_params.arate, bitrate, sound_buf_pos);
        if ((len2 % 8) == 0 || len > max_write_size)
        {
            static short *buf2 = nullptr;
            len2 = Resampler::resample(&buf2, RESAMPLED_FREQ, reinterpret_cast<short *>(m_sound_buf), m_params.arate,
                                       bitrate, sound_buf_pos);

            if (len2 > 0)
            {
                if ((len2 % 4) != 0)
                {
                    g_view_logger->info("[CaptureManager]: Warning: Possible stereo sound error detected.\n");
                    fprintf(stderr, "[CaptureManager]: Warning: Possible stereo sound error detected.\n");
                }

                const BOOL ok = (0 == AVIStreamWrite(m_sound_stream, m_sample, len2 / m_sound_format.nBlockAlign, buf2,
                                                     len2, 0, NULL, NULL));
                m_sample += len2 / m_sound_format.nBlockAlign;
                m_avi_file_size += len2;

                if (!ok)
                {
                    DialogService::show_dialog(L"Audio output failure!\nA call to addAudioData() (AVIStreamWrite) "
                                               L"failed.\nPerhaps you ran out of memory?",
                                               L"AVI Encoder", fsvc_error);
                    return false;
                }
            }
            sound_buf_pos = 0;
        }
    }

    if (len <= 0)
    {
        return true;
    }

    if (static_cast<unsigned int>(sound_buf_pos + len) > SOUND_BUF_SIZE * sizeof(char))
    {
        DialogService::show_dialog(L"Sound buffer overflow!\nCapture will be stopped.", L"AVI Encoder", fsvc_error);
        return false;
    }

#ifdef _DEBUG
    long double pro = (long double)(sound_buf_pos + len) * 100 / (SOUND_BUF_SIZE * sizeof(char));
    if (pro > 75) g_view_logger->warn("Audio buffer almost full ({:.0f}%)!", pro);
#endif

    memcpy(m_sound_buf + sound_buf_pos, (char *)buf, len);
    sound_buf_pos += len;

#ifndef VFW_ENCODER_PARALLELIZED
    m_audio_frame += ((len / 4) / (long double)m_params.arate) *
                     g_main_ctx.core_ctx->vr_get_vis_per_second(g_main_ctx.core_ctx->vr_get_rom_header()->Country_code);

#endif

    return true;
}

bool VFWEncoder::append_video_impl(uint8_t *image)
{
    LONG written_len = 0;
    BOOL ret = AVIStreamWrite(m_compressed_video_stream, m_frame++, 1, image, m_info_hdr.biSizeImage, AVIIF_KEYFRAME,
                              NULL, &written_len);
    m_avi_file_size += written_len;

    if (ret != 0)
    {
        DialogService::show_dialog(
            L"Video output failure!\nA call to AVIStreamWrite failed.\nPerhaps you ran out of memory?", L"AVI Encoder",
            fsvc_error);
    }

    return ret == 0;
}

bool VFWEncoder::enqueue_work(WorkItem item)
{
    const size_t item_bytes = item.data.size();

    std::unique_lock lock(m_work_mutex);
    m_work_cv.wait(lock, [this, item_bytes]() {
        return m_worker_failed || !m_worker_running ||
               (m_work_queue.size() < MAX_PENDING_WORK_ITEMS &&
                (m_pending_work_bytes + item_bytes) <= MAX_PENDING_WORK_BYTES);
    });

    if (m_worker_failed || !m_worker_running || m_worker_stop_requested) return false;

    m_pending_work_bytes += item_bytes;
    m_work_queue.emplace_back(std::move(item));
    m_work_cv.notify_all();
    return true;
}

void VFWEncoder::wait_for_all_work()
{
    std::unique_lock lock(m_work_mutex);
    m_work_drained_cv.wait(lock,
                           [this]() { return (m_work_queue.empty() && m_work_in_flight == 0) || !m_worker_running; });
}

void VFWEncoder::worker_loop()
{
    for (;;)
    {
        WorkItem item{};
        {
            std::unique_lock lock(m_work_mutex);
            m_work_cv.wait(lock, [this]() { return m_worker_stop_requested || !m_work_queue.empty(); });

            if (m_work_queue.empty())
            {
                if (m_worker_stop_requested)
                {
                    m_worker_running = false;
                    m_work_cv.notify_all();
                    m_work_drained_cv.notify_all();
                    return;
                }
                continue;
            }

            item = std::move(m_work_queue.front());
            m_pending_work_bytes -= item.data.size();
            m_work_queue.pop_front();
            ++m_work_in_flight;
            m_work_cv.notify_all();
        }

        bool ok = true;
        if (item.type == WorkType::Video)
        {
            for (size_t i = 0; i < item.frame_count; ++i)
            {
                ok = append_video_impl(item.data.data());
                if (!ok) break;
            }
        }
        else
        {
            const int write_size = m_params.arate * 2;

            if (g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::Video) ||
                g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::None))
            {
                double_t desync = item.desync;

                if (g_config.synchronization_mode == (int)CaptureManager::Sync::None) desync = 0.0;

                if (desync > 1.0)
                {
                    g_view_logger->info("[CaptureManager]: Correcting for A/V desynchronization of %+Lf frames\n",
                                        desync);
                    int len3 = (int)(m_params.arate / (long double)g_main_ctx.core_ctx->vr_get_vis_per_second(
                                                          g_main_ctx.core_ctx->vr_get_rom_header()->Country_code)) *
                               (int)desync;
                    len3 <<= 2;
                    const int empty_size = len3 > write_size ? write_size : len3;

                    for (int i = 0; i < empty_size; i += 4)
                        *reinterpret_cast<long *>(m_sound_buf_empty + i) = last_sound;

                    while (len3 > write_size)
                    {
                        ok =
                            write_sound(m_sound_buf_empty, write_size, m_params.arate, write_size, FALSE, item.bitrate);
                        if (!ok) break;

                        len3 -= write_size;
                    }
                    if (ok) ok = write_sound(m_sound_buf_empty, len3, m_params.arate, write_size, FALSE, item.bitrate);
                }
                else if (desync <= -10.0)
                {
                    g_view_logger->info("[CaptureManager]: Waiting from A/V desynchronization of %+Lf frames\n",
                                        desync);
                }
            }

            if (ok)
            {
                ok = write_sound(item.data.empty() ? nullptr : item.data.data(), static_cast<int>(item.data.size()),
                                 m_params.arate, write_size, item.force ? TRUE : FALSE, item.bitrate);
            }

            if (ok && !item.data.empty())
            {
                last_sound = *(reinterpret_cast<long *>(item.data.data() + item.data.size()) - 1);
            }
        }

        {
            std::scoped_lock lock(m_work_mutex);
            --m_work_in_flight;
            if (m_work_queue.empty() && m_work_in_flight == 0) m_work_drained_cv.notify_all();
        }

        if (!ok)
        {
            std::scoped_lock lock(m_work_mutex);
            m_worker_failed = true;
            m_worker_stop_requested = true;
            m_work_queue.clear();
            m_pending_work_bytes = 0;
            m_work_in_flight = 0;
            m_worker_running = false;
            m_work_cv.notify_all();
            m_work_drained_cv.notify_all();
            return;
        }
    }
}

bool VFWEncoder::save_options() const
{
    FILE *f = nullptr;
    if (fopen_s(&f, "avi.cfg", "wb"))
    {
        g_view_logger->error("[AVIEncoder] {} fopen() failed", __func__);
        return false;
    }

    if (fwrite(&m_avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f) < 1)
    {
        g_view_logger->error("[AVIEncoder] {} fwrite(m_avi_options) failed", __func__);
        (void)fclose(f);
        return false;
    }

    if (m_avi_options.lpParms)
    {
        if (fwrite(m_avi_options.lpParms, m_avi_options.cbParms, 1, f) < 1)
        {
            g_view_logger->error("[AVIEncoder] {} fwrite(m_avi_options->lpParms) failed", __func__);
            (void)fclose(f);
            return false;
        }
    }

    (void)fclose(f);

    return true;
}

bool VFWEncoder::load_options()
{
    FILE *f = nullptr;
    if (fopen_s(&f, "avi.cfg", "rb"))
    {
        return false;
    }
    fseek(f, 0, SEEK_END);

    // Too small...
    if (ftell(f) < sizeof(AVICOMPRESSOPTIONS)) goto error;

    fseek(f, 0, SEEK_SET);

    fread(&m_avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f);

    {
        void *params = malloc(m_avi_options.cbParms);
        const bool has_params = fread(params, m_avi_options.cbParms, 1, f) == 1;
        if (has_params)
        {
            m_avi_options.lpParms = params;
        }
        free(params);
    }

    (void)fclose(f);
    return true;

error:
    fclose(f);
    return false;
}
