/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "EmuContext.hpp"
#include <IOUtils.hpp>
#include <MiscHelpers.hpp>

#include <atomic>
#include <print>
#include <ranges>

#include <QQmlEngine>
#include <QThread>
#include <QIcon>
#include <QUrl>

#include <QtUtils.hpp>
#include <QJSFunctions.hpp>

static std::atomic<EmuContext *> g_core_instance = nullptr;

static core_cfg g_core_cfg{};
static core_params g_core_params{};

static void set_core_instance(EmuContext *ptr)
{
    EmuContext *expect = nullptr;
    if (!g_core_instance.compare_exchange_strong(expect, ptr))
        throw std::logic_error("EmuContext should not be created twice!");
}

EmuContext::EmuContext(QObject *parent)
    : QObject(parent), m_core_cfg(&g_core_cfg), m_core_params(&g_core_params), m_core_ctx(nullptr)
{
    set_core_instance(this);

    m_core_params->cfg = m_core_cfg;

#pragma region Directories
    m_core_params->submit_task = [](const auto &cb) {
        // Defer to the stdlib's thread pool.
        (void)std::async(cb);
    };
    m_core_params->get_saves_directory = [] {
        static auto s_save_path = IOUtils::exe_path().parent_path() / "saves";
        if (!std::filesystem::is_directory(s_save_path)) std::filesystem::create_directories(s_save_path);
        return s_save_path;
    };
    m_core_params->get_backups_directory = [] {
        static auto s_backups_path = IOUtils::exe_path().parent_path() / "backups";
        if (!std::filesystem::is_directory(s_backups_path)) std::filesystem::create_directories(s_backups_path);
        return s_backups_path;
    };
    m_core_params->get_summercart_path = []() { return IOUtils::exe_path().parent_path() / "saves/cart.vhd"; };
#pragma endregion

#pragma region Logging
    m_core_params->log_trace = [](std::string_view msg) { std::println(stderr, "[TRACE] {}", msg); };
    m_core_params->log_info = [](std::string_view msg) { std::println(stderr, "[INFO]  {}", msg); };
    m_core_params->log_warn = [](std::string_view msg) { std::println(stderr, "[WARN]  {}", msg); };
    m_core_params->log_error = [](std::string_view msg) { std::println(stderr, "[ERROR] {}", msg); };
#pragma endregion

#pragma region Plugin integration

    m_core_params->callbacks.emu_starting = [&] { m_plugins.value().emu_started(*m_core_params); };
    m_core_params->callbacks.emu_stopped = [&] {
        m_plugins.value().emu_stopped(*m_core_params);
        m_plugins.reset();
        m_fn_read_video = nullptr;
    };

    m_core_params->load_plugins = [&] {
        try
        {
            auto video_plugin = Plugin(BuiltinTAS::PluginID::TASVideo);
            auto audio_plugin = Plugin(BuiltinTAS::PluginID::TASAudio);
            auto input_plugin = Plugin(BuiltinTAS::PluginID::DummyInput);
            auto rsp_plugin = Plugin(BuiltinTAS::PluginID::TASRSP);
            m_plugins.emplace(std::move(video_plugin), std::move(audio_plugin), std::move(input_plugin),
                              std::move(rsp_plugin));

            // Read video functions
            m_fn_read_video = (M64RRSpec::PtrReadVideo)m_plugins->video().load_symbol("M64RRReadVideo");
            return true;
        }
        catch (const std::exception &err)
        {
            std::println(stderr, "[ERROR] Plugin load failed: {}", err.what());
            return false;
        }
    };
    m_core_params->initiate_plugins = [&] { m_plugins.value().initiate_plugins(m_core_ctx, *m_core_params); };
    CoreUtil::clear_plugin_funcs(*m_core_params);

#pragma endregion

#pragma region Dialog service
    m_core_params->show_multiple_choice_dialog = [&](std::string_view id, const std::vector<std::string> &choices,
                                                     const char *str, const char *title,
                                                     core_dialog_type type) -> size_t {
        std::promise<size_t> promise;
        auto future = promise.get_future();

        auto done_callback = QJSFunctions::to_js_function(
            qmlEngine(this), [promise = std::move(promise)](uint32_t result) mutable { promise.set_value(result); });

        auto qt_choices = choices | std::views::transform(QString::fromStdString) | std::ranges::to<QList>();
        QMetaObject::invokeMethod(this, &EmuContext::openMultiDialog, done_callback, QAnyStringView(title),
                                  QAnyStringView(str), qt_choices, CoreDialogType::from_core(type));
        future.wait();
        return future.get();
    };
    m_core_params->show_ask_dialog = [&](std::string_view id, const char *str, const char *title,
                                         bool warning) -> bool {
        std::promise<bool> promise;
        auto future = promise.get_future();

        auto done_callback = QJSFunctions::to_js_function(
            qmlEngine(this), [promise = std::move(promise)](bool value) mutable { promise.set_value(value); });

        QMetaObject::invokeMethod(this, &EmuContext::openAskDialog, done_callback, QAnyStringView(title),
                                  QAnyStringView(str), warning ? CoreDialogType::Warning : CoreDialogType::Information);

        future.wait();
        return future.get();
    };
    m_core_params->show_dialog = [&](const char *str, const char *title, core_dialog_type type) {
        std::promise<void> promise;
        auto future = promise.get_future();

        auto done_callback = QJSFunctions::to_js_function(
            qmlEngine(this), [promise = std::move(promise)] mutable { promise.set_value(); });

        QMetaObject::invokeMethod(this, &EmuContext::openInfoDialog, done_callback, QAnyStringView(title),
                                  QAnyStringView(str), CoreDialogType::from_core(type));

        future.wait();
    };
#pragma endregion

#pragma region Signals and properties
    m_core_params->update_screen = [&] {
        std::println("next frame!");
        QMetaObject::invokeMethod(this, &EmuContext::updateScreen);
    };

    m_core_params->callbacks.emu_launched_changed = [&](bool value) {
        QMetaObject::invokeMethod(this, &EmuContext::emuLaunchedChanged, value);
    };
#pragma endregion
    core_create(m_core_params, &m_core_ctx);
}

EmuContext::~EmuContext()
{
    // ensure ROM is closed
    m_core_ctx->vr_close_rom(true);
}

EmuContext *EmuContext::instance()
{
    return g_core_instance;
}

CoreResult::Value EmuContext::vrStartROM(const QUrl &url) const
{
    std::filesystem::path path = url.toLocalFile().toStdU16String();
    return CoreResult::from_core(m_core_ctx->vr_start_rom(path));
}

CoreResult::Value EmuContext::vrCloseROM(bool resetVCR) const
{
    return CoreResult::from_core(m_core_ctx->vr_close_rom(resetVCR));
}

bool EmuContext::isEmuLaunched() const
{
    return m_core_ctx->vr_get_launched();
}

void EmuContext::readVideoOutput(QImage &image)
{
    int32_t width = 0;
    int32_t height = 0;
    m_fn_read_video(image.bits(), &width, &height);

    if (width != image.width() && height != image.height())
        throw std::logic_error("Video output not pre-sized correctly!");
}

void CoreUtil::clear_plugin_funcs(core_params &params)
{
    params.video_process_dlist = [](auto...) {};
    params.video_process_rdp_list = [](auto...) {};
    params.video_show_cfb = [](auto...) {};
    params.video_vi_status_changed = [](auto...) {};
    params.video_vi_width_changed = [](auto...) {};
    params.video_get_video_size = [](auto...) {};
    params.video_fb_read = [](auto...) {};
    params.video_fb_write = [](auto...) {};
    params.video_fb_get_frame_buffer_info = [](auto...) {};
    params.audio_ai_dacrate_changed = [](auto...) {};
    params.audio_ai_len_changed = [](auto...) {};
    params.audio_ai_read_length = [](auto...) { return 0; };
    params.audio_process_alist = [](auto...) {};
    params.input_controller_command = [](auto...) {};
    params.input_get_keys = [](auto...) {};
    params.input_set_keys = [](auto...) {};
    params.input_read_controller = [](auto...) {};
    params.rsp_do_rsp_cycles = [](auto...) { return 0; };
}