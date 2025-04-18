/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <DialogService.h>

#include <capture/EncodingManager.h>
#include <capture/Resampler.h>
#include <capture/encoders/AVIEncoder.h>
#include <gui/Loggers.h>
#include <gui/Main.h>


std::wstring AVIEncoder::start(Params params)
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
    m_info_hdr.biBitCount = 24;
    m_info_hdr.biCompression = BI_RGB;
    m_info_hdr.biSizeImage = params.width * params.height * 3;
    m_info_hdr.biXPelsPerMeter = 0;
    m_info_hdr.biYPelsPerMeter = 0;
    m_info_hdr.biClrUsed = 0;
    m_info_hdr.biClrImportant = 0;

    DeleteFile(params.path.wstring().c_str());

    AVIFileInit();
    if (AVIFileOpen(&m_avi_file, params.path.wstring().c_str(), OF_WRITE | OF_CREATE, NULL))
    {
        stop();
        return L"Failed to open output file.";
    }

    ZeroMemory(&m_video_stream_hdr, sizeof(AVISTREAMINFO));
    m_video_stream_hdr.fccType = streamtypeVIDEO;
    m_video_stream_hdr.dwScale = 1;
    m_video_stream_hdr.dwRate = params.fps;
    m_video_stream_hdr.dwSuggestedBufferSize = 0;
    if (AVIFileCreateStream(m_avi_file, &m_video_stream, &m_video_stream_hdr))
    {
        stop();
        return L"Failed to create video file stream.";
    }

    if (params.ask_for_encoding_settings && !m_splitting)
    {
        if (!AVISaveOptions(g_main_hwnd, 0, 1, &m_video_stream, &m_avi_options))
        {
            return L"Failed to save options.";
        }

        if (!save_options())
        {
            stop();
            return L"Failed to save options.";
        }
    }
    else
    {
        if (!load_options())
        {
            return L"Failed to load options. Verify that the encoding preset file is present.";
        }
    }

    if (AVIMakeCompressedStream(&m_compressed_video_stream, m_video_stream, m_avi_options, NULL) != AVIERR_OK)
    {
        stop();
        return L"Failed to make video compressed stream.";
    }

    if (AVIStreamSetFormat(m_compressed_video_stream, 0, &m_info_hdr, m_info_hdr.biSize + m_info_hdr.biClrUsed * sizeof(RGBQUAD)) != AVIERR_OK)
    {
        stop();
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
        stop();
        return L"Failed to create audio stream.";
    }
    if (AVIStreamSetFormat(m_sound_stream, 0, &m_sound_format, sizeof(WAVEFORMATEX)) != AVIERR_OK)
    {
        stop();
        return L"Failed to set audio stream format.";
    }

    memset(m_sound_buf_empty, 0, sizeof(m_sound_buf_empty));
    memset(m_sound_buf, 0, sizeof(m_sound_buf));
    last_sound = 0;

    return L"";
}

bool AVIEncoder::stop()
{
    // TODO: Move bitrate to params
    write_sound(nullptr, 0, RESAMPLED_FREQ, RESAMPLED_FREQ * 2, TRUE, 16);

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

    return true;
}


bool AVIEncoder::append_video(uint8_t* image)
{
    if (g_config.synchronization_mode != static_cast<int>(EncodingManager::Sync::Audio) && g_config.synchronization_mode != static_cast<int>(EncodingManager::Sync::None))
    {
        return true;
    }

    bool result = true;

    // AUDIO SYNC
    // This type of syncing assumes the audio is authoratative, and drops or duplicates frames to keep the video as close to
    // it as possible. Some games stop updating the screen entirely at certain points, such as loading zones, which will cause
    // audio to drift away by default. This method of syncing prevents this, at the cost of the video feed possibly freezing or jumping
    // (though in practice this rarely happens - usually a loading scene just appears shorter or something).

    int audio_frames = (int)(m_audio_frame - m_video_frame + 0.1);
    // i've seen a few games only do ~0.98 frames of audio for a frame, let's account for that here

    if (g_config.synchronization_mode == (int)EncodingManager::Sync::Audio)
    {
        if (audio_frames < 0)
        {
            g_view_logger->error("[AVIEncoder] Audio frames became negative!");
            return false;
        }

        if (audio_frames == 0)
        {
            g_view_logger->warn("Dropped Frame! a/v: %Lg/%Lg", m_video_frame, m_audio_frame);
        }
        else if (audio_frames > 0)
        {
            result = append_video_impl(image);
            m_video_frame += 1.0;
            audio_frames--;
        }

        // can this actually happen?
        while (audio_frames > 0)
        {
            result = append_video_impl(image);
            g_view_logger->warn("Duped Frame! a/v: %Lg/%Lg", m_video_frame, m_audio_frame);
            m_video_frame += 1.0;
            audio_frames--;
        }
    }
    else
    {
        result = append_video_impl(image);
        m_video_frame += 1.0;
    }

    return result;
}

bool AVIEncoder::append_audio(uint8_t* audio, size_t length, uint8_t bitrate)
{
    const int write_size = m_params.arate * 2;

    if (g_config.synchronization_mode == static_cast<int>(EncodingManager::Sync::Video) || g_config.synchronization_mode == static_cast<int>(EncodingManager::Sync::None))
    {
        // VIDEO SYNC
        // This is the original syncing code, which adds silence to the audio track to get it to line up with video.
        // The N64 appears to have the ability to arbitrarily disable its sound processing facilities and no audio samples
        // are generated. When this happens, the video track will drift away from the audio. This can happen at load boundaries
        // in some games, for example.
        //
        // The only new difference here is that the desync flag is checked for being greater than 1.0 instead of 0.
        // This is because the audio and video in mupen tend to always be diverged just a little bit, but stay in sync
        // over time. Checking if desync is not 0 causes the audio stream to to get thrashed which results in clicks
        // and pops.

        double_t desync = m_video_frame - m_audio_frame;

        if (g_config.synchronization_mode == (int)EncodingManager::Sync::None) // HACK
            desync = 0.0;

        if (desync > 1.0)
        {
            g_view_logger->info(
            "[EncodingManager]: Correcting for A/V desynchronization of %+Lf frames\n",
            desync);
            int len3 = (int)(m_params.arate / (long double)core_vr_get_vis_per_second(core_vr_get_rom_header()->Country_code)) * (int)desync;
            len3 <<= 2;
            const int empty_size =
            len3 > write_size ? write_size : len3;

            for (int i = 0; i < empty_size; i += 4)
                *reinterpret_cast<long*>(m_sound_buf_empty + i) =
                last_sound;

            while (len3 > write_size)
            {
                write_sound(m_sound_buf_empty, write_size, m_params.arate, write_size, FALSE, bitrate);
                len3 -= write_size;
            }
            write_sound(m_sound_buf_empty, len3, m_params.arate, write_size, FALSE, bitrate);
        }
        else if (desync <= -10.0)
        {
            g_view_logger->info(
            "[EncodingManager]: Waiting from A/V desynchronization of %+Lf frames\n",
            desync);
        }
    }

    write_sound(audio, length, m_params.arate, write_size, FALSE, bitrate);
    last_sound = *(reinterpret_cast<long*>(audio + length) - 1);

    return true;
}

bool AVIEncoder::write_sound(uint8_t* buf, int len, const int min_write_size, const int max_write_size, const BOOL force, uint8_t bitrate)
{
    if ((len <= 0 && !force) || len > max_write_size)
        return false;

    if (sound_buf_pos + len > min_write_size || force)
    {
        int len2 = rsmp_get_resample_len(RESAMPLED_FREQ, m_params.arate, bitrate, sound_buf_pos);
        if ((len2 % 8) == 0 || len > max_write_size)
        {
            static short* buf2 = nullptr;
            len2 = rsmp_resample(&buf2, RESAMPLED_FREQ, reinterpret_cast<short*>(m_sound_buf), m_params.arate, bitrate, sound_buf_pos);

            if (len2 > 0)
            {
                if ((len2 % 4) != 0)
                {
                    g_view_logger->info(
                    "[EncodingManager]: Warning: Possible stereo sound error detected.\n");
                    fprintf(
                    stderr,
                    "[EncodingManager]: Warning: Possible stereo sound error detected.\n");
                }

                const BOOL ok = (0 == AVIStreamWrite(m_sound_stream, m_sample, len2 / m_sound_format.nBlockAlign, buf2, len2, 0, NULL, NULL));
                m_sample += len2 / m_sound_format.nBlockAlign;
                m_avi_file_size += len2;

                if (!ok)
                {
                    DialogService::show_dialog(L"Audio output failure!\nA call to addAudioData() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?", L"AVI Encoder", fsvc_error);
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
    if (pro > 75)
        g_view_logger->warn("Audio buffer almost full ({:.0f}%)!", pro);
#endif

    memcpy(m_sound_buf + sound_buf_pos, (char*)buf, len);
    sound_buf_pos += len;
    m_audio_frame += ((len / 4) / (long double)m_params.arate) *
    core_vr_get_vis_per_second(core_vr_get_rom_header()->Country_code);


    return true;
}

bool AVIEncoder::append_video_impl(uint8_t* image)
{
    LONG written_len;
    BOOL ret = AVIStreamWrite(m_compressed_video_stream, m_frame++, 1, image, m_info_hdr.biSizeImage, AVIIF_KEYFRAME, NULL, &written_len);
    m_avi_file_size += written_len;
    return !ret;
}

bool AVIEncoder::save_options() const
{
    FILE* f = fopen("avi.cfg", "wb");

    if (!f)
    {
        g_view_logger->error("[AVIEncoder] {} fopen() failed", __func__);
        return false;
    }

    if (fwrite(m_avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f) < 1)
    {
        g_view_logger->error("[AVIEncoder] {} fwrite(m_avi_options) failed", __func__);
        (void)fclose(f);
        return false;
    }

    if (fwrite(m_avi_options->lpParms, m_avi_options->cbParms, 1, f) < 1)
    {
        g_view_logger->error("[AVIEncoder] {} fwrite(m_avi_options->lpParms) failed", __func__);
        (void)fclose(f);
        return false;
    }

    (void)fclose(f);

    return true;
}

bool AVIEncoder::load_options()
{
    FILE* f = fopen("avi.cfg", "rb");

    if (!f)
        return false;

    fseek(f, 0, SEEK_END);

    // Too small...
    if (ftell(f) < sizeof(AVICOMPRESSOPTIONS))
        goto error;

    fseek(f, 0, SEEK_SET);

    fread(m_avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f);

    {
        void* moreOptions = malloc(m_avi_options->cbParms);
        fread(moreOptions, m_avi_options->cbParms, 1, f);
        m_avi_options->lpParms = moreOptions;
    }

    fclose(f);
    return true;

error:
    fclose(f);
    return false;
}
