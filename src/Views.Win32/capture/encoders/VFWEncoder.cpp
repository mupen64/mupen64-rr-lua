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
    m_last_sound = 0;

    return std::nullopt;
}

bool VFWEncoder::stop_impl(const bool fail_stop)
{
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

    return true;
}

bool VFWEncoder::stop()
{
    return this->stop_impl(false);
}

uint32_t shortHash(const uint8_t *d, size_t n)
{
    uint32_t h = 2166136261u;
    while (n--) h = (h ^ *d++) * 16777619u;
    return h;
}

bool VFWEncoder::append_video(uint8_t *image)
{
    const auto hash = shortHash(image, m_params.width * m_params.height * 4);

    if (g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::Video) ||
        g_config.synchronization_mode == static_cast<int>(CaptureManager::Sync::None))
    {
        g_view_logger->trace(L"video buffer hash {:08X}", hash);

        if (!append_video_impl(image)) return false;
        m_video_frame++;
        return true;
    }

    const double drift = m_audio_frame - static_cast<double>(m_video_frame);
    constexpr double DRIFT_THRESHOLD = 3.0;

    // Video is ahead of audio, drop frame
    if (drift < -DRIFT_THRESHOLD)
    {
        return true;
    }

    if (!append_video_impl(image)) return false;
    m_video_frame++;

    // Audio is ahead of video, duplicate frame
    if (drift > DRIFT_THRESHOLD)
    {
        if (!append_video_impl(image)) return false;
        m_video_frame++;
    }

    return true;
}

bool VFWEncoder::append_audio(uint8_t *audio, size_t length, uint8_t bitrate)
{
    write_sound(audio, length, bitrate);
    std::memcpy(&m_last_sound, audio + length - sizeof(uint32_t), sizeof(uint32_t));

    return true;
}

bool VFWEncoder::write_sound(uint8_t *buf, int len, uint8_t bitrate)
{
    if (len <= 0) return true;

    const auto fill_percentage = (double)(sound_buf_pos + len) * 100.0 / SOUND_BUF_SIZE;
    RT_ASSERT(fill_percentage <= 80, L"Audio buffer overflowed");

    memcpy(m_sound_buf + sound_buf_pos, buf, len);
    sound_buf_pos += len;

    m_in_sample += len / m_sound_format.nBlockAlign;
    m_audio_frame = (double)m_in_sample * m_params.fps / m_params.arate;

    int expected_len = Resampler::get_resample_len(RESAMPLED_FREQ, m_params.arate, bitrate, sound_buf_pos);

    if (expected_len <= 0 || (expected_len % 8) != 0) return true;

    int resampled_len = Resampler::resample(&m_resampled_sound, RESAMPLED_FREQ, reinterpret_cast<short *>(m_sound_buf),
                                            m_params.arate, bitrate, sound_buf_pos);

    if (resampled_len <= 0) return true;

    RT_ASSERT((resampled_len % 4) == 0, L"Resampled audio is not stereo-aligned");

    BOOL ok = (0 == AVIStreamWrite(m_sound_stream, m_sample, resampled_len / m_sound_format.nBlockAlign,
                                   m_resampled_sound, resampled_len, 0, NULL, NULL));

    if (!ok)
    {
        DialogService::show_dialog(L"Audio output failure!\n"
                                   L"A call to AVIStreamWrite failed.\n"
                                   L"Perhaps you ran out of memory?",
                                   L"AVI Encoder", fsvc_error);
        return false;
    }

    m_sample += resampled_len / m_sound_format.nBlockAlign;

    m_avi_file_size += resampled_len;
    sound_buf_pos = 0;

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

std::wstring VFWEncoder::get_desired_extension() const
{
    return L".avi";
}
