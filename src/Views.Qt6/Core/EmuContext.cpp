/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "EmuContext.hpp"

#include <print>

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
    : QObject(parent), m_core_cfg(&g_core_cfg), m_core_params(&g_core_params), m_core_ctx(nullptr),
      m_plugins(std::nullopt), m_fn_read_video(nullptr)
{
    set_core_instance(this);

    m_core_params->cfg = m_core_cfg;

#pragma region Directories
    m_core_params->submit_task = [&](const std::function<void()> &cb) { m_task_pool.start(cb); };
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
            m_plugins.emplace(
                std::move(video_plugin), std::move(audio_plugin), std::move(input_plugin), std::move(rsp_plugin));

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
                                                     CoreMessageTone type) -> size_t {
        std::promise<size_t> promise;
        auto future = promise.get_future();

        QMetaObject::invokeMethod(
            this, [=, this, promise = std::move(promise), str = QString(str), title = QString(title)] mutable {
                // JS objects should be instantiated on the event thread
                auto done_callback = QJSFunctions::to_js_function(qmlEngine(this),
                    [promise = std::move(promise)](uint32_t result) mutable { promise.set_value(result); });
                auto qt_choices = choices | std::views::transform(QString::fromStdString) | std::ranges::to<QList>();

                openMultiDialog(done_callback, title, str, qt_choices, CoreDialogType::from_core(type));
            });

        return future.get();
    };
    m_core_params->show_ask_dialog = [&](std::string_view id, const char *str, const char *title,
                                         bool warning) -> bool {
        std::promise<bool> promise;
        auto future = promise.get_future();

        QMetaObject::invokeMethod(this, [=, this, promise = std::move(promise), str = QString(str),
                                            title = QString(title)] mutable {
            // JS objects should be instantiated on the event thread
            auto done_callback = QJSFunctions::to_js_function(qmlEngine(this),
                [promise = std::move(promise)](uint32_t result) mutable { promise.set_value(result); });

            openAskDialog(done_callback, title, str, warning ? CoreDialogType::Warning : CoreDialogType::Information);
        });

        return future.get();
    };
    m_core_params->show_dialog = [&](const char *str, const char *title, CoreMessageTone type) {
        std::promise<void> promise;
        auto future = promise.get_future();

        QMetaObject::invokeMethod(
            this, [=, this, promise = std::move(promise), str = QString(str), title = QString(title)] mutable {
                // JS objects should be instantiated on the event thread
                auto done_callback = QJSFunctions::to_js_function(
                    qmlEngine(this), [promise = std::move(promise)] mutable { promise.set_value(); });

                openAskDialog(done_callback, title, str, CoreDialogType::from_core(type));
            });

        future.get();
    };
#pragma endregion

#pragma region Signals and properties
    // propagate signals from core
    m_core_params->update_screen = [&] { QMetaObject::invokeMethod(this, &EmuContext::updateScreen); };
    m_core_params->callbacks.emu_launched_changed = [&](bool value) {
        QMetaObject::invokeMethod(this, &EmuContext::launchedChanged, value);
    };
    m_core_params->callbacks.emu_paused_changed = [&](bool value) {
        QMetaObject::invokeMethod(this, &EmuContext::pausedChanged, value);
    };
    m_core_params->callbacks.core_executing_changed = [&](bool value) {
        QMetaObject::invokeMethod(this, &EmuContext::coreExecutingChanged, value);
    };

    // propagate signals to core
    connect(
        this, &EmuContext::speedModifierChanged, this, [&](int32_t) { m_core_ctx->vr_on_speed_modifier_changed(); });
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

// vr_* functions
// ==========================

CoreResult::Value EmuContext::startROM(const QUrl &url)
{
    std::filesystem::path path = url.toLocalFile().toStdU16String();
    return CoreResult::from_core(m_core_ctx->vr_start_rom(path));
}

CoreResult::Value EmuContext::closeROM(bool resetVCR)
{
    return CoreResult::from_core(m_core_ctx->vr_close_rom(resetVCR));
}

CoreResult::Value EmuContext::resetROM(bool resetSaveData, bool stopVCR)
{
    return CoreResult::from_core(m_core_ctx->vr_reset_rom(resetSaveData, stopVCR));
}

void EmuContext::invalidateVisuals()
{
    m_core_ctx->vr_invalidate_visuals();
}

void EmuContext::frameAdvance(size_t frames)
{
    m_core_ctx->vr_frame_advance(frames);
}

// vr_* properties
// ==========================

bool EmuContext::isLaunched() const
{
    return m_core_ctx->vr_get_launched();
}

bool EmuContext::isPaused() const
{
    return m_core_ctx->vr_get_paused();
}
void EmuContext::setPaused(bool paused)
{
    if (paused)
        m_core_ctx->vr_pause_emu();
    else
        m_core_ctx->vr_resume_emu();
}

bool EmuContext::isCoreExecuting()
{
    return m_core_ctx->vr_get_core_executing();
}

bool EmuContext::isGSButton() const
{
    return m_core_ctx->vr_get_gs_button();
}
void EmuContext::setGSButton(bool pressed)
{
    if (pressed != m_core_ctx->vr_get_gs_button())
    {
        m_core_ctx->vr_set_gs_button(pressed);
        gsButtonChanged(pressed);
    }
}

// st_* functions
// ==========================

// -> st_do_memory (to save slot)
void EmuContext::saveSlot(uint32_t index)
{
    if (index >= NUM_SAVE_SLOTS) return;
    // TODO implement based on config directories
}

// -> st_do_file
void EmuContext::saveFile(const QUrl &url)
{
    std::filesystem::path path = url.toLocalFile().toStdU16String();
    std::println("saving to {}", path.string());

    // Save operations must be issued asynchronously as they lock a mutex.
    // To keep operations from running on the wrong frame, we also block the core from
    // advancing until the operation is queued.
    m_core_ctx->vr_wait_increment();
    m_task_pool.start([=, this] {
        m_core_ctx->vr_wait_decrement();
        m_core_ctx->st_do_file(path, core_st_job_save, nullptr, false);
    });
}

// -> st_do_memory (to save slot)
void EmuContext::loadSlot(uint32_t index)
{
    if (index >= NUM_SAVE_SLOTS) return;
    // TODO implement based on config directories
}

// -> st_do_file
void EmuContext::loadFile(const QUrl &url)
{
    std::filesystem::path path = url.toLocalFile().toStdU16String();
    // see saveFile()
    m_core_ctx->vr_wait_increment();
    m_task_pool.start([=, this] {
        m_core_ctx->vr_wait_decrement();
        m_core_ctx->st_do_file(path, core_st_job_load, nullptr, false);
    });
}

// core_cfg properties
// ==========================
int32_t EmuContext::speedModifier()
{
    return m_core_cfg->fps_modifier;
}
void EmuContext::setSpeedModifier(int32_t valueIn)
{
    int32_t value = std::clamp<int32_t>(valueIn, 5, 1000);
    if (value != m_core_cfg->fps_modifier)
    {
        m_core_cfg->fps_modifier = value;
        speedModifierChanged(value);
    }
}

// Misc. functions
// ==========================

void EmuContext::readVideoOutput(QImage &image)
{
    int32_t width = 0;
    int32_t height = 0;
    m_fn_read_video(nullptr, &width, &height);

    // reallocate if needed
    if (image.width() != width || image.height() != height)
    {
        image = QImage(width, height, QImage::Format_ARGB32);
        // make the image orange in case read_video isn't working
        image.fill(0x00FF8000);
    }

    m_fn_read_video(image.bits(), nullptr, nullptr);
    // std::println("pixel: {:08X}", image.pixel(320, 240));
}

// Internal utilities
// ==========================

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
