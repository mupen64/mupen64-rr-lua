/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <AsyncExecutor.h>
#include <Config.h>
#include <DialogService.h>
#include <Messenger.h>
#include <capture/EncodingManager.h>
#include <capture/encoders/AVIEncoder.h>
#include <capture/encoders/Encoder.h>
#include <capture/encoders/FFmpegEncoder.h>
#include <Loggers.h>
#include <Main.h>
#include <features/Dispatcher.h>
#include <features/MGECompositor.h>
#include <lua/LuaRenderer.h>
#include <lua/LuaConsole.h>

namespace EncodingManager
{
    std::filesystem::path m_current_path;

    // 0x30018
    int m_audio_freq = 33000;
    int m_audio_bitrate = 16;
    long double m_video_frame = 0;
    long double m_audio_frame = 0;
    size_t m_total_frames = 0;

    // Video buffer, allocated once when recording starts and freed when it ends.
    uint8_t* m_video_buf = nullptr;
    int32_t m_video_width;
    int32_t m_video_height;

    std::atomic m_capturing = false;
    cfg_encoder_type m_encoder_type;
    std::unique_ptr<Encoder> m_encoder;
    std::recursive_mutex m_mutex;

    HDC hy_main_dc = nullptr;
    HDC hy_dc = nullptr;
    HBITMAP hy_bmp = nullptr;

    void readscreen_plugin(int32_t* width = nullptr, int32_t* height = nullptr)
    {
        if (core_vr_get_mge_available())
        {
            MGECompositor::copy_video(m_video_buf);
            MGECompositor::get_video_size(width, height);
        }
        else
        {
            void* buf = nullptr;
            int32_t w;
            int32_t h;
            g_core.plugin_funcs.video_read_screen(&buf, &w, &h);
            memcpy(m_video_buf, buf, w * h * 3);
            g_core.plugin_funcs.video_dll_crt_free(buf);

            if (width)
            {
                *width = w;
            }
            if (height)
            {
                *height = h;
            }
        }
    }

    void readscreen_window()
    {
        g_main_window_dispatcher->invoke([] {
            HDC dc = GetDC(g_main_hwnd);
            HDC compat_dc = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, m_video_width, m_video_height);

            SelectObject(compat_dc, bitmap);

            BitBlt(compat_dc, 0, 0, m_video_width, m_video_height, dc, 0, 0, SRCCOPY);

            BITMAPINFO bmp_info{};
            bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmp_info.bmiHeader.biWidth = m_video_width;
            bmp_info.bmiHeader.biHeight = m_video_height;
            bmp_info.bmiHeader.biPlanes = 1;
            bmp_info.bmiHeader.biBitCount = 24;
            bmp_info.bmiHeader.biCompression = BI_RGB;

            GetDIBits(compat_dc, bitmap, 0, m_video_height, m_video_buf, &bmp_info, DIB_RGB_COLORS);

            SelectObject(compat_dc, nullptr);
            DeleteObject(bitmap);
            DeleteDC(compat_dc);
            ReleaseDC(g_main_hwnd, dc);
        });
    }

    void readscreen_desktop()
    {
        g_main_window_dispatcher->invoke([] {
            POINT pt{};
            ClientToScreen(g_main_hwnd, &pt);

            HDC dc = GetDC(nullptr);
            HDC compat_dc = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, m_video_width, m_video_height);

            SelectObject(compat_dc, bitmap);

            BitBlt(compat_dc, 0, 0, m_video_width, m_video_height, dc, pt.x, pt.y, SRCCOPY);

            BITMAPINFO bmp_info{};
            bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmp_info.bmiHeader.biWidth = m_video_width;
            bmp_info.bmiHeader.biHeight = m_video_height;
            bmp_info.bmiHeader.biPlanes = 1;
            bmp_info.bmiHeader.biBitCount = 24;
            bmp_info.bmiHeader.biCompression = BI_RGB;

            GetDIBits(compat_dc, bitmap, 0, m_video_height, m_video_buf, &bmp_info, DIB_RGB_COLORS);

            SelectObject(compat_dc, nullptr);
            DeleteObject(bitmap);
            DeleteDC(compat_dc);
            ReleaseDC(nullptr, dc);
        });
    }

    void readscreen_hybrid()
    {
        int32_t raw_video_width, raw_video_height;
        readscreen_plugin(&raw_video_width, &raw_video_height);

        // UI resources, must be accessed from UI thread
        // To avoid GDI weirdness with cross-thread resources, we do all GDI work on UI thread.
        g_main_window_dispatcher->invoke([&] {
            // Since atupdatescreen might not have occured for a long time, we force it now.
            // This avoids "outdated" visuals, which are otherwise acceptable during normal gameplay, being blitted to the video stream.
            LuaRenderer::repaint_visuals();

            GdiFlush();

            if (!hy_dc)
            {
                g_view_logger->trace("Creating hybrid capture resources...");
                hy_main_dc = GetDC(g_main_hwnd);
                hy_dc = CreateCompatibleDC(hy_main_dc);
                hy_bmp = CreateCompatibleBitmap(hy_main_dc, m_video_width, m_video_height);
                SelectObject(hy_dc, hy_bmp);
            }

            {
                BITMAPINFO bmp_info{};
                bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmp_info.bmiHeader.biPlanes = 1;
                bmp_info.bmiHeader.biBitCount = 24;
                bmp_info.bmiHeader.biWidth = raw_video_width;
                bmp_info.bmiHeader.biHeight = raw_video_height;
                bmp_info.bmiHeader.biCompression = BI_RGB;

                // Copy the raw readscreen output
                StretchDIBits(hy_dc,
                              0,
                              0,
                              raw_video_width,
                              raw_video_height,
                              0,
                              0,
                              raw_video_width,
                              raw_video_height,
                              m_video_buf,
                              &bmp_info,
                              DIB_RGB_COLORS,
                              SRCCOPY);
            }

            // First, composite the lua's dxgi surfaces
            for (auto& lua : g_lua_environments)
            {
                if (lua->rctx.presenter)
                {
                    lua->rctx.presenter->blit(hy_dc, {0, 0, (LONG)lua->rctx.presenter->size().width, (LONG)lua->rctx.presenter->size().height});
                }
            }

            // Then, blit the GDI back DCs
            for (auto& lua : g_lua_environments)
            {
                TransparentBlt(hy_dc, 0, 0, lua->rctx.dc_size.width, lua->rctx.dc_size.height, lua->rctx.gdi_back_dc, 0, 0, lua->rctx.dc_size.width, lua->rctx.dc_size.height, LUA_GDI_COLOR_MASK);
            }

            BITMAPINFO bmp_info{};
            bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmp_info.bmiHeader.biWidth = m_video_width;
            bmp_info.bmiHeader.biHeight = m_video_height;
            bmp_info.bmiHeader.biPlanes = 1;
            bmp_info.bmiHeader.biBitCount = 24;
            bmp_info.bmiHeader.biCompression = BI_RGB;

            GetDIBits(hy_dc, hy_bmp, 0, m_video_height, m_video_buf, &bmp_info, DIB_RGB_COLORS);
        });
    }

    void read_screen()
    {
        if (g_config.capture_mode == 0)
        {
            if (!core_vr_get_mge_available() && !g_core.plugin_funcs.video_read_screen)
            {
                DialogService::show_dialog(L"The current video plugin has no readScreen implementation.\nPlugin capture is not possible.", L"Capture", fsvc_error);
                stop_capture();
                return;
            }

            readscreen_plugin();
        }
        else if (g_config.capture_mode == 1)
        {
            readscreen_window();
        }
        else if (g_config.capture_mode == 2)
        {
            readscreen_desktop();
        }
        else if (g_config.capture_mode == 3)
        {
            if (!core_vr_get_mge_available() && !g_core.plugin_funcs.video_read_screen)
            {
                DialogService::show_dialog(L"The current video plugin has no readScreen implementation.\nHybrid capture is not possible.", L"Capture", fsvc_error);
                stop_capture();
                return;
            }
            readscreen_hybrid();
        }
        else
        {
            assert(false);
        }
    }

    void get_video_dimensions(int32_t* width, int32_t* height)
    {
        if (g_config.capture_mode == 0)
        {
            if (core_vr_get_mge_available())
            {
                MGECompositor::get_video_size(width, height);
            }
            else if (g_core.plugin_funcs.video_get_video_size)
            {
                g_core.plugin_funcs.video_get_video_size(width, height);
            }
            else
            {
                void* buf = nullptr;
                g_core.plugin_funcs.video_read_screen(&buf, width, height);
                g_core.plugin_funcs.video_dll_crt_free(buf);
            }
        }
        else if (g_config.capture_mode == 1 || g_config.capture_mode == 2 || g_config.capture_mode == 3)
        {
            const auto info = get_window_info();
            *width = info.width & ~3;
            *height = info.height & ~3;
        }
        else
        {
            assert(false);
        }
    }

    bool stop_capture_impl()
    {
        std::lock_guard lock(m_mutex);

        if (!is_capturing())
        {
            return true;
        }

        if (!m_encoder->stop())
        {
            DialogService::show_dialog(L"Failed to stop encoding.", L"Capture", fsvc_error);
            return false;
        }

        m_encoder.release();

        g_main_window_dispatcher->invoke([] {
            SelectObject(hy_dc, nullptr);

            DeleteObject(hy_bmp);
            hy_bmp = nullptr;

            DeleteDC(hy_dc);
            hy_dc = nullptr;

            ReleaseDC(g_main_hwnd, hy_main_dc);
            hy_main_dc = nullptr;
        });

        m_capturing = false;
        g_config.core.render_throttling = true;

        Messenger::broadcast(Messenger::Message::CapturingChanged, false);

        g_view_logger->info("[EncodingManager]: Capture finished.");
        return true;
    }

    bool start_capture_impl(std::filesystem::path path, cfg_encoder_type encoder_type, const bool ask_for_encoding_settings)
    {
        std::lock_guard lock(m_mutex);

        g_view_logger->info("[EncodingManager]: Starting capture at {} x {}...", m_video_width, m_video_height);

        if (is_capturing())
        {
            if (!stop_capture_impl())
            {
                g_view_logger->info("[EncodingManager]: Couldn't start capture because the previous capture couldn't be stopped.");
                return false;
            }
        }

        switch (encoder_type)
        {
        case ENCODER_VFW:
            m_encoder = std::make_unique<AVIEncoder>();
            break;
        case ENCODER_FFMPEG:
            m_encoder = std::make_unique<FFmpegEncoder>();
            break;
        default:
            assert(false);
        }

        m_encoder_type = encoder_type;
        m_current_path = path;

        if (encoder_type == ENCODER_FFMPEG)
        {
            m_current_path.replace_extension(".mp4");
        }

        m_video_frame = 0.0;
        m_audio_frame = 0.0;
        m_total_frames = 0;

        free(m_video_buf);
        get_video_dimensions(&m_video_width, &m_video_height);
        m_video_buf = (uint8_t*)malloc(m_video_width * m_video_height * 3);

        auto result = m_encoder->start(Encoder::Params{
        .path = m_current_path,
        .width = (uint32_t)m_video_width,
        .height = (uint32_t)m_video_height,
        .fps = core_vr_get_vis_per_second(core_vr_get_rom_header()->Country_code),
        .arate = (uint32_t)m_audio_freq,
        .ask_for_encoding_settings = ask_for_encoding_settings,
        });

        if (!result.empty())
        {
            DialogService::show_dialog(result.c_str(), L"Capture", fsvc_error);
            return false;
        }

        m_capturing = true;
        g_config.core.render_throttling = false;

        Messenger::broadcast(Messenger::Message::CapturingChanged, true);

        return true;
    }

    void start_capture(std::filesystem::path path, cfg_encoder_type encoder_type, const bool ask_for_encoding_settings, const std::function<void(bool)>& callback)
    {
        core_vr_wait_increment();
        AsyncExecutor::invoke_async([=] {
            const auto result = start_capture_impl(path, encoder_type, ask_for_encoding_settings);
            if (callback)
            {
                callback(result);
            }
            core_vr_wait_decrement();
        });
    }

    void stop_capture(const std::function<void(bool)>& callback)
    {
        core_vr_wait_increment();
        AsyncExecutor::invoke_async([=] {
            const auto result = stop_capture_impl();
            if (callback)
            {
                callback(result);
            }
            core_vr_wait_decrement();
        });
    }

    void at_vi()
    {
        std::lock_guard lock(m_mutex);

        if (!m_capturing)
        {
            return;
        }

        if (g_config.capture_delay)
        {
            Sleep(g_config.capture_delay);
        }

        read_screen();

        if (m_encoder->append_video(m_video_buf))
        {
            m_total_frames++;
            return;
        }

        DialogService::show_dialog(
        L"Failed to append frame to video.\nPerhaps you ran out of memory?",
        L"Capture",
        fsvc_error);
        stop_capture();
    }

    void ai_len_changed()
    {
        std::lock_guard lock(m_mutex);

        const auto p = reinterpret_cast<short*>((char*)g_core.rdram + (g_core.ai_register->ai_dram_addr & 0xFFFFFF));
        const auto buf = (char*)p;
        const int ai_len = (int)g_core.ai_register->ai_len;

        m_audio_bitrate = (int)g_core.ai_register->ai_bitrate + 1;

        if (!m_capturing)
        {
            return;
        }

        if (ai_len <= 0)
            return;

        if (!m_encoder->append_audio(reinterpret_cast<uint8_t*>(buf), ai_len, m_audio_bitrate))
        {
            DialogService::show_dialog(
            L"Failed to append audio data.\nCapture will be stopped.",
            L"Capture",
            fsvc_error);
            stop_capture();
        }
    }

    void ai_dacrate_changed(std::any data)
    {
        auto type = std::any_cast<core_system_type>(data);

        if (m_capturing)
        {
            DialogService::show_dialog(L"Audio frequency changed during capture.\r\nThe capture will be stopped.", L"Capture", fsvc_error);
            stop_capture();
            return;
        }

        m_audio_bitrate = (int)g_core.ai_register->ai_bitrate + 1;

        switch (type)
        {
        case sys_ntsc:
            m_audio_freq = (int)(48681812 / (g_core.ai_register->ai_dacrate + 1));
            break;
        case sys_pal:
            m_audio_freq = (int)(49656530 / (g_core.ai_register->ai_dacrate + 1));
            break;
        default:
            assert(false);
            break;
        }
        g_view_logger->info("[EncodingManager] m_audio_freq: {}", m_audio_freq);
    }

    size_t get_video_frame()
    {
        return m_total_frames;
    }

    std::filesystem::path get_current_path()
    {
        return m_current_path;
    }

    bool is_capturing()
    {
        return m_capturing;
    }

    void init()
    {
        Messenger::subscribe(Messenger::Message::DacrateChanged, ai_dacrate_changed);
    }
} // namespace EncodingManager
