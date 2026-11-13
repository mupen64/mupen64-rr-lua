/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "EmuContext.hpp"

#include <print>

#include <QIcon>
#include <QQmlEngine>
#include <QThread>
#include <QThreadPool>
#include <QUrl>

static std::atomic<EmuContext *> g_core_instance = nullptr;

static CoreCfg g_core_cfg{};
static CoreParams g_core_params{};

static void set_core_instance(EmuContext *ptr)
{
    EmuContext *expect = nullptr;
    if (!g_core_instance.compare_exchange_strong(expect, ptr))
        throw std::logic_error("EmuContext should not be created twice!");
}

EmuContext::EmuContext(QQuickItem *parent)
    : QQuickItem(parent), m_core_cfg(&g_core_cfg), m_core_params(&g_core_params), m_core_ctx(nullptr),
      m_plugins(std::nullopt), m_fn_read_video(nullptr), m_options(new EmuOptions(this)), m_paths(new EmuPaths(this)),
      m_rom_manager(nullptr)
{
    set_core_instance(this);

    m_core_params->cfg = m_core_cfg;

#pragma region General integration
    m_core_params->submit_task = [&](const std::function<void()> &cb) { QThreadPool::globalInstance()->start(cb); };
    m_core_params->find_available_rom =
        [&](const std::function<bool(const CoreROMHeader &)> &predicate) -> std::filesystem::path {
        // fetch ROM list from UI. If the UI is loading the ROM list, this will wait for that loading to finish.
        std::promise<RomManager::RomList> rom_list_promise;
        auto rom_list_future = rom_list_promise.get_future();
        QMetaObject::invokeMethod(this, [this, rom_list_promise = std::move(rom_list_promise)] mutable {
            auto func = [rom_list_promise = std::move(rom_list_promise)](
                            const RomManager::RomList &list) mutable { rom_list_promise.set_value(list); };
            m_rom_manager->requestRomData(std::move(func));
        });

        // Filter according to the core-provided predicate.
        auto rom_list = rom_list_future.get();
        auto candidate = std::ranges::find_if(
            rom_list, [&predicate](const auto &rom_data) { return predicate(rom_data->rawHeader()); });
        if (candidate == rom_list.end()) return {};

        // Convert from QString to std::filesystem::path
        auto qstr_path = (*candidate)->path();
        return std::filesystem::path{std::u16string_view{qstr_path}};
    };
#pragma endregion

#pragma region Directories
    m_core_params->get_saves_directory = [this] { return m_paths->saveDirStdPath(); };
    m_core_params->get_backups_directory = [this] { return m_paths->backupDirStdPath(); };
    m_core_params->get_summercart_directory = [this] { return m_paths->saveDirStdPath(); };
    m_core_params->get_summercart_path = [this]() { return m_paths->saveDirStdPath() / "cart.vhd"; };
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
        std::lock_guard lock(m_read_video_mutex);
        m_plugins.value().emu_stopped(*m_core_params);
        m_plugins.reset();
        m_fn_read_video = nullptr;
    };

    m_core_params->load_plugins = [&] {
        try
        {
            auto video_plugin = Plugin(BuiltinTAS::PluginID::TASVideo);
            auto audio_plugin = Plugin(BuiltinTAS::PluginID::TASAudio);
            auto input_plugin = Plugin(BuiltinTAS::PluginID::TASInput);
            auto rsp_plugin = Plugin(BuiltinTAS::PluginID::TASRSP);
            std::lock_guard lock(m_read_video_mutex);
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
        auto qt_choices = choices | std::views::transform(QString::fromStdString) | std::ranges::to<QList>();
        std::println("fire! {}", id);
        return this->startMultiDialog(title, str, qt_choices, QmlCoreMessageTone::from_core(type)).get();
    };
    m_core_params->show_ask_dialog = [&](std::string_view id, const char *str, const char *title,
                                         bool warning) -> bool {
        return this->startAskDialog(title, str, warning ? QmlCoreMessageTone::Warn : QmlCoreMessageTone::Info).get();
    };
    m_core_params->show_dialog = [&](const char *str, const char *title, CoreMessageTone type) {
        this->startInfoDialog(title, str, QmlCoreMessageTone::from_core(type)).get();
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
    m_core_params->callbacks.task_changed = [&](CoreVCRTask value) {
        QMetaObject::invokeMethod(this, &EmuContext::vcrTaskChanged, QmlCoreVCRTask::from_core(value));
    };

    // propagate signals to core
    connect(
        this, &EmuContext::speedModifierChanged, this, [&](int32_t) { m_core_ctx->vr_on_speed_modifier_changed(); });
#pragma endregion

    core_create(m_core_params, &m_core_ctx);
}

EmuContext::~EmuContext()
{
    cancelPendingDialogs();

    // ensure ROM is closed
    m_core_ctx->vr_close_rom(true);
    g_core_instance.store(nullptr);
}

EmuContext *EmuContext::instance()
{
    return g_core_instance;
}

// vr_* functions
// ==========================

void EmuContext::startROM(const QString &pathIn)
{
    std::filesystem::path path = std::u16string_view{pathIn};
    QPointer<EmuContext> self(this);
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([self, coreCtx, path] {
        auto result = coreCtx->vr_start_rom(path);
        if (self) self->startDialogForCoreResult(result);
    });
}

void EmuContext::closeROM(bool resetVCR)
{
    QPointer<EmuContext> self(this);
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([self, coreCtx, resetVCR] {
        auto result = coreCtx->vr_close_rom(resetVCR);
        if (self) self->startDialogForCoreResult(result);
    });
}

void EmuContext::resetROM(bool resetSaveData, bool stopVCR)
{
    QPointer<EmuContext> self(this);
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([self, coreCtx, resetSaveData, stopVCR] {
        auto result = coreCtx->vr_reset_rom(resetSaveData, stopVCR);
        if (self) self->startDialogForCoreResult(result);
    });
}

void EmuContext::invalidateVisuals()
{
    m_core_ctx->vr_invalidate_visuals();
}

void EmuContext::frameAdvance(size_t frames)
{
    m_core_ctx->vr_frame_advance(frames);
}

// st_* functions
// ==========================

// -> st_do_memory (to save slot)
void EmuContext::saveSlot(uint32_t index)
{
    if (index >= num_save_slots) return;
    // TODO implement based on config directories
}

// -> st_do_file
void EmuContext::saveFile(const QString &pathIn)
{
    std::filesystem::path path = std::u16string_view{pathIn};
    std::println("saving to {}", path.string());

    // Save operations must be issued asynchronously as they lock a mutex.
    // To keep operations from running on the wrong frame, we also block the core from
    // advancing until the operation is queued.
    m_core_ctx->vr_wait_increment();
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([coreCtx, path] {
        coreCtx->vr_wait_decrement();
        coreCtx->st_do_file(path, CoreSTJob::Save, nullptr, false);
    });
}

// -> st_do_memory (to save slot)
void EmuContext::loadSlot(uint32_t index)
{
    if (index >= num_save_slots) return;
    // TODO implement based on config directories
}

// -> st_do_file
void EmuContext::loadFile(const QString &pathIn)
{
    std::filesystem::path path = std::u16string_view{pathIn};
    // see saveFile()
    m_core_ctx->vr_wait_increment();
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([coreCtx, path] {
        coreCtx->vr_wait_decrement();
        coreCtx->st_do_file(path, CoreSTJob::Load, nullptr, false);
    });
}

// vcr_* functions
// ==========================

// -> vcr_start_playback
void EmuContext::vcrStartPlayback(const QString &path, const QVariant &author, const QVariant &description)
{
    QThreadPool::globalInstance()->start(
        [self = QPointer<EmuContext>(this), path = (std::filesystem::path)(std::u16string_view)path,
            author = author.isNull() ? std::nullopt : std::make_optional(author.toString().toStdString()),
            description =
                description.isNull() ? std::nullopt : std::make_optional(description.toString().toStdString())] {
            self->m_core_ctx->vcr_replace_author_info(path, author, description);
            const auto result = self->m_core_ctx->vcr_start_playback(path);
            if (result != CoreResult::Res_Ok) self->startDialogForCoreResult(result);
        });
}

// -> vcr_start_recording
void EmuContext::vcrStartRecording(
    const QString &path_qstr, uint32_t flags_quint, const QString &author_qstr, const QString &description_qstr)
{
    if (path_qstr.isEmpty()) return;

    // operation needs to be executed asynchronously to avoid blocking the UI thread
    QThreadPool::globalInstance()->start(
        [self = QPointer<EmuContext>(this), path = (std::filesystem::path)(std::u16string_view)path_qstr,
            flags = (uint16_t)flags_quint, author = author_qstr.toStdString(),
            description = description_qstr.toStdString()

    ] {
            const auto file_info = self->m_core_ctx->vcr_get_generated_file_info(path, flags);
            const auto any_file_exists = std::filesystem::exists(file_info.movie_path) ||
                                         (!file_info.st_path.empty() && std::filesystem::exists(file_info.st_path)) ||
                                         (!file_info.cht_path.empty() && std::filesystem::exists(file_info.cht_path));

            // show warning if *any* files could be overwritten
            if (any_file_exists)
            {
                //% "Overwrite Movie"
                auto title = qtTrId("core.dialog.movieOverwriteWarning.title");
                //% "The specified movie file (or one of its accompanying files) already exists."
                //% " Do you want to overwrite it?"
                auto content = qtTrId("core.dialog.movieOverwriteWarning");
                const auto overwrite = self->startAskDialog(title, content, QmlCoreMessageTone::Warn).get();

                if (!overwrite) return;
            }

            self->m_core_ctx->vr_wait_increment();
            auto vcr_result = self->m_core_ctx->vcr_start_record(path, flags, author, description);
            self->m_core_ctx->vr_wait_decrement();
            if (vcr_result != CoreResult::Res_Ok)
            {
                self->startDialogForCoreResult(vcr_result);
                return;
            }
        });
}

// -> vcr_continue_recording
void EmuContext::vcrContinueRecording()
{
    QPointer<EmuContext> self(this);
    CoreCtx *coreCtx = m_core_ctx;

    coreCtx->vr_wait_increment();
    QThreadPool::globalInstance()->start([self, coreCtx] {
        auto result = coreCtx->vcr_continue_recording();
        coreCtx->vr_wait_decrement();
        if (self) self->startDialogForCoreResult(result);
    });
}

// -> vcr_stop_all
void EmuContext::vcrStopAll()
{
    m_core_ctx->vr_wait_increment();
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([coreCtx] {
        coreCtx->vcr_stop_all();
        coreCtx->vr_wait_decrement();
    });
}

// -> vcr_write_backup
void EmuContext::vcrWriteBackup()
{
    QPointer<EmuContext> self(this);
    CoreCtx *coreCtx = m_core_ctx;
    QThreadPool::globalInstance()->start([self, coreCtx] {
        auto result = coreCtx->vcr_write_backup();
        if (self) self->startDialogForCoreResult(result);
    });
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

QmlCoreSpeedMode::Value EmuContext::speedMode() const
{
    return QmlCoreSpeedMode::from_core(m_core_ctx->vr_get_speed_mode());
}
void EmuContext::setSpeedMode(QmlCoreSpeedMode::Value speedMode)
{
    if (speedMode != QmlCoreSpeedMode::from_core(m_core_ctx->vr_get_speed_mode()))
    {
        m_core_ctx->vr_set_speed_mode(QmlCoreSpeedMode::to_core(speedMode));
        speedModeChanged(speedMode);
    }
}

QmlCoreVCRTask::Value EmuContext::vcrTask() const
{
    return QmlCoreVCRTask::from_core(m_core_ctx->vcr_get_task());
}

// CoreCfg properties
// ==========================
int32_t EmuContext::speedModifier() const
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

// -> .vcr_readonly
bool EmuContext::isVcrReadonly() const
{
    return m_core_cfg->vcr_readonly;
}
void EmuContext::setVcrReadonly(bool value)
{
    if (m_core_cfg->vcr_readonly == value) return;
    m_core_cfg->vcr_readonly = value;
    vcrReadonlyChanged(value);
}

// -> .is_movie_loop_enabled
bool EmuContext::isMovieLoopEnabled() const
{
    return (bool)m_core_cfg->is_movie_loop_enabled;
}
void EmuContext::setMovieLoopEnabled(bool value)
{
    if (m_core_cfg->is_movie_loop_enabled == value) return;
    m_core_cfg->is_movie_loop_enabled = value;
    movieLoopEnabledChanged(value);
}

// -> .wait_at_movie_end
bool EmuContext::isWaitAtMovieEnd() const
{
    return (bool)m_core_cfg->wait_at_movie_end;
}
void EmuContext::setWaitAtMovieEnd(bool value)
{
    if (m_core_cfg->wait_at_movie_end == value) return;
    m_core_cfg->wait_at_movie_end = value;
    waitAtMovieEndChanged(value);
}

int EmuContext::registerDialog(std::move_only_function<void(int)> finish)
{
    const int id = m_next_dialog_id++;
    m_pending_dialogs.emplace(id, PendingDialog{std::move(finish)});
    return id;
}

std::future<size_t> EmuContext::startMultiDialog(
    QAnyStringView title, QAnyStringView content, const QList<QString> &choices, QmlCoreMessageTone::Value type)
{
    std::promise<size_t> promise;
    auto future = promise.get_future();

    QMetaObject::invokeMethod(this, [=, this, promise = std::move(promise), title = title.toString(),
                                        content = content.toString(), choices = choices] mutable {
        const int id = registerDialog(
            [promise = std::move(promise)](int result) mutable { promise.set_value(static_cast<size_t>(result)); });
        openMultiDialog(id, title, content, choices, type);
    });

    return future;
}

std::future<bool> EmuContext::startAskDialog(
    QAnyStringView title, QAnyStringView content, QmlCoreMessageTone::Value type)
{
    std::promise<bool> promise;
    auto future = promise.get_future();

    QMetaObject::invokeMethod(
        this, [=, this, promise = std::move(promise), title = title.toString(), content = content.toString()] mutable {
            const int id =
                registerDialog([promise = std::move(promise)](int result) mutable { promise.set_value(result == 1); });
            openAskDialog(id, title, content, type);
        });

    return future;
}
std::future<void> EmuContext::startInfoDialog(
    QAnyStringView title, QAnyStringView content, QmlCoreMessageTone::Value type)
{
    std::promise<void> promise;
    auto future = promise.get_future();

    QMetaObject::invokeMethod(
        this, [=, this, promise = std::move(promise), title = title.toString(), content = content.toString()] mutable {
            const int id = registerDialog([promise = std::move(promise)](int) mutable { promise.set_value(); });
            openInfoDialog(id, title, content, type);
        });

    return future;
}

void EmuContext::finishDialog(int id, int result)
{
    auto it = m_pending_dialogs.find(id);
    if (it == m_pending_dialogs.end()) return;

    auto finish = std::move(it->second.finish);
    m_pending_dialogs.erase(it);
    finish(result);
}

void EmuContext::cancelPendingDialogs()
{
    auto pending = std::move(m_pending_dialogs);
    m_pending_dialogs.clear();

    for (auto &entry : pending) entry.second.finish(-1);
}

// Misc. functions
// ==========================
EmuOptions *EmuContext::options()
{
    return m_options;
}

EmuPaths *EmuContext::paths()
{
    return m_paths;
}

void EmuContext::readVideoOutput(QImage &image)
{
    std::lock_guard lock(m_read_video_mutex);
    if (m_fn_read_video == nullptr) return;

    int32_t width = 0;
    int32_t height = 0;
    m_fn_read_video(nullptr, &width, &height);

    // reallocate if needed
    if (image.width() != width || image.height() != height)
    {
        image = QImage(width, height, QImage::Format_RGB32);
        // make the image orange in case read_video isn't working
        image.fill(0x00FF8000);
    }

    m_fn_read_video(image.bits(), nullptr, nullptr);
    // std::println("pixel: {:08X}", image.pixel(320, 240));
}

// Misc. properties
// ==========================

RomManager *EmuContext::romManager()
{
    return m_rom_manager;
}
void EmuContext::setRomManager(RomManager *value)
{
    if (value == m_rom_manager) return;
    m_rom_manager = value;
    romManagerChanged();
}

// Internal utilities
// ==========================

void EmuContext::startDialogForCoreResult(CoreResult result)
{
    using namespace Qt::Literals;

    if (result == CoreResult::Res_Ok) return;

    auto message = QmlCoreResult::messageCpp(QmlCoreResult::from_core(result));
    if (!message.has_value()) return;

    auto [module, error] = message.value();

    auto title = u"%1 Error %2"_s.arg(module, static_cast<int32_t>(result));
    this->startInfoDialog(title, error, QmlCoreMessageTone::Error);
}

void CoreUtil::clear_plugin_funcs(CoreParams &params)
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
