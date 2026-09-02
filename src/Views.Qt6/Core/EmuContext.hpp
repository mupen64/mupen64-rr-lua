/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QThreadPool>
#include <QUrl>
#include <qqmlintegration.h>

#include <m64rr/API.hpp>
#include "plugin/Plugin.hpp"

#include "CoreEnums.hpp"

class EmuOptions;
class EmuPaths;

/**
 * @brief QML-owned singleton holding the core and related objects.
 * Implemented as a non-singleton, but will throw a tantrum if instantiated more than once.
 */
class EmuContext : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    friend class EmuOptions;

    // core_ctx properties
    Q_PROPERTY(bool launched READ isLaunched NOTIFY launchedChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool coreExecuting READ isCoreExecuting NOTIFY coreExecutingChanged)
    Q_PROPERTY(bool gsButton READ isGSButton WRITE setGSButton NOTIFY gsButtonChanged)

    // core_cfg properties
    Q_PROPERTY(int32_t speedModifier READ speedModifier WRITE setSpeedModifier NOTIFY speedModifierChanged)

    // extra properties
    Q_PROPERTY(EmuOptions *options READ options)
    Q_PROPERTY(EmuPaths *paths READ paths)
  public:
    static constexpr size_t NUM_SAVE_SLOTS = 10;

    EmuContext(QObject *parent = nullptr);
    virtual ~EmuContext();

    static EmuContext *instance();

    static core_ctx *rawContext()
    {
        auto *inst = instance();
        return (inst != nullptr) ? inst->m_core_ctx : nullptr;
    }

    // vr_* functions
    // ==========================

    // -> vr_start_rom
    Q_INVOKABLE CoreResult::Value startROM(const QUrl &url);

    // -> vr_close_rom
    Q_INVOKABLE CoreResult::Value closeROM(bool resetVCR = true);

    // -> vr_reset_rom
    Q_INVOKABLE CoreResult::Value resetROM(bool resetSaveData, bool stopVCR);

    // -> vr_invalidate_visuals
    Q_INVOKABLE void invalidateVisuals();

    // -> vr_frame_advance
    Q_INVOKABLE void frameAdvance(size_t frames);

    // vr_* properties
    // ==========================

    // -> vr_get_launched
    bool isLaunched() const;

    // -> vr_get_paused
    bool isPaused() const;
    // -> vr_pause_emu/vr_resume_emu
    void setPaused(bool paused);

    // -> vr_get_core_executing
    bool isCoreExecuting();

    // -> vr_get_gs_button
    bool isGSButton() const;
    // -> vr_set_gs_button
    void setGSButton(bool pressed);

    // st_* functions
    // ==========================

    // -> st_do_file (to save slot)
    Q_INVOKABLE void saveSlot(uint32_t index);

    // -> st_do_file
    Q_INVOKABLE void saveFile(const QUrl &url);

    // -> st_do_file (to save slot)
    Q_INVOKABLE void loadSlot(uint32_t index);

    // -> st_do_file
    Q_INVOKABLE void loadFile(const QUrl &url);

    // core_cfg properties
    // ==========================

    // -> .fps_modifier
    int32_t speedModifier();
    void setSpeedModifier(int32_t value);

    // Misc. functions
    // ==========================

    /**
     * @brief Gets the EmuOptions object associated with this context.
     * This contains many config options that don't make sense being in the main object.
     */
    Q_INVOKABLE EmuOptions *options();

    /**
     * @brief Gets the EmuOptions object associated with this context.
     * This contains many config options that don't make sense being in the main object.
     */
    Q_INVOKABLE EmuPaths *paths();

    /**
     * @brief Calls the video plugin's `ReadVideo` function, reading out to an image.
     * @note May reallocate the image if needed.
     *
     * @param image The image to read to.
     */
    void readVideoOutput(QImage &image);

  signals:

    // vr_* properties
    // ==========================

    // -> callbacks.emu_launched_changed
    void launchedChanged(bool value);

    // -> callbacks.emu_paused_changed
    void pausedChanged(bool value);

    // -> callbacks.core_executing_changed
    void coreExecutingChanged(bool value);

    // -> set_gs_button() called
    void gsButtonChanged(bool value);

    // core_cfg properties
    // ==========================

    // -> .fps_modifier changed
    void speedModifierChanged(int32_t value);

    // extra properties
    // ==========================
    void configSourceChanged(const QJSValue &value);

    // Graphics signals
    // ============================================

    /**
     * @brief Requests that the window be resized (when the emulator is open).
     *
     * @param width The requested width.
     * @param height The requested height.
     */
    void gfxRequestSize(uint32_t width, uint32_t height);

    /**
     * @brief Requests that the video output be updated.
     */
    void updateScreen();

    // Dialog service (to be handled by GUI)
    // ============================================

    /**
     * @brief Opens a multiple-choice dialog.
     *
     * @param done (type: `void(size_t)`) Function to be called when finished.
     * @param title The dialog's title.
     * @param content The dialog's content text.
     * @param choices A list of choices to use for the bottom buttons.
     * @param type The dialog's type. Used to display an icon next to the text.
     */
    void openMultiDialog(QJSValue done, QAnyStringView title, QAnyStringView content, const QList<QString> &choices,
        CoreDialogType::Value type);

    /**
     * @brief Opens a yes/no dialog.
     *
     * @param done (type: `void(bool result)`) Function to be called when finished.
     * @param title The dialog's title.
     * @param content The dialog's content text.
     * @param type The dialog's type. Used to display an icon next to the text.
     */
    void openAskDialog(QJSValue done, QAnyStringView title, QAnyStringView content, CoreDialogType::Value type);

    /**
     * @brief Opens an info dialog.
     *
     * @param done (type: `void()`) Function to be called when finished.
     * @param title The dialog's title.
     * @param content The dialog's content text.
     * @param type The dialog's type. Used to display an icon next to the text.
     */
    void openInfoDialog(QJSValue done, QAnyStringView title, QAnyStringView content, CoreDialogType::Value type);

  private:
    core_cfg *m_core_cfg;
    core_params *m_core_params;
    core_ctx *m_core_ctx;

    std::optional<PluginSet> m_plugins;
    M64RRSpec::PtrReadVideo m_fn_read_video;

    QThreadPool m_task_pool;

    EmuOptions *m_options;
    EmuPaths *m_paths;
};

/**
 * @brief Qt bindings for parts of core_cfg that are only used for configuration.
 */
class EmuOptions : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(int coreType READ coreType WRITE setCoreType NOTIFY coreTypeChanged)
    Q_PROPERTY(bool stUndoLoad READ stUndoLoad WRITE setStUndoLoad NOTIFY stUndoLoadChanged)
    Q_PROPERTY(int maxLag READ maxLag WRITE setMaxLag NOTIFY maxLagChanged)
    Q_PROPERTY(bool wiiVCEmulation READ wiiVCEmulation WRITE setWiiVCEmulation NOTIFY wiiVCEmulationChanged)
    Q_PROPERTY(bool rcpLagEmulation READ rcpLagEmulation WRITE setRcpLagEmulation NOTIFY rcpLagEmulationChanged)
    Q_PROPERTY(double cpuCF READ cpuCF WRITE setCpuCF NOTIFY cpuCFChanged)
    Q_PROPERTY(double rcpLagFactor READ rcpLagFactor WRITE setRcpLagFactor NOTIFY rcpLagFactorChanged)
    Q_PROPERTY(bool floatExceptionEmulation READ floatExceptionEmulation WRITE setFloatExceptionEmulation NOTIFY
            floatExceptionEmulationChanged)
    Q_PROPERTY(bool useSummercart READ useSummercart WRITE setUseSummercart NOTIFY useSummercartChanged)
    Q_PROPERTY(bool stScreenshot READ stScreenshot WRITE setStScreenshot NOTIFY stScreenshotChanged)
    Q_PROPERTY(bool stLZ4 READ stLZ4 WRITE setStLz4 NOTIFY stLZ4Changed)
    Q_PROPERTY(int romCacheSize READ romCacheSize WRITE setRomCacheSize NOTIFY romCacheSizeChanged)
    Q_PROPERTY(bool audioDelayEnabled READ audioDelayEnabled WRITE setAudioDelayEnabled NOTIFY audioDelayEnabledChanged)
    Q_PROPERTY(bool compiledJumpEnabled READ compiledJumpEnabled WRITE setCompiledJumpEnabled NOTIFY
            compiledJumpEnabledChanged)
    Q_PROPERTY(bool ceqsNaNAccurate READ ceqsNaNAccurate WRITE setCeqsNaNAccurate NOTIFY ceqsNaNAccurateChanged)
    Q_PROPERTY(bool accurateRDPCompletion READ accurateRDPCompletion WRITE setAccurateRdpCompletion NOTIFY
            accurateRDPCompletionChanged)
    Q_PROPERTY(bool vcrBackups READ vcrBackups WRITE setVcrBackups NOTIFY vcrBackupsChanged)
    Q_PROPERTY(bool vcrWriteExtendedFormat READ vcrWriteExtendedFormat WRITE setVcrWriteExtendedFormat NOTIFY
            vcrWriteExtendedFormatChanged)
  public:
    EmuOptions(EmuContext *parent) : QObject(parent), m_context(parent) {}

    int coreType() { return m_context->m_core_cfg->core_type; }
    void setCoreType(int value)
    {
        if (value == m_context->m_core_cfg->core_type) return;
        m_context->m_core_cfg->core_type = value;
        coreTypeChanged();
    }
    bool stUndoLoad() { return m_context->m_core_cfg->st_undo_load; }
    void setStUndoLoad(bool value)
    {
        if (value == m_context->m_core_cfg->st_undo_load) return;
        m_context->m_core_cfg->st_undo_load = value;
        stUndoLoadChanged();
    }
    int maxLag() { return m_context->m_core_cfg->max_lag; }
    void setMaxLag(int value)
    {
        if (value == m_context->m_core_cfg->max_lag) return;
        m_context->m_core_cfg->max_lag = value;
        maxLagChanged();
    }
    bool wiiVCEmulation() { return m_context->m_core_cfg->wii_vc_emulation; }
    void setWiiVCEmulation(bool value)
    {
        if (value == m_context->m_core_cfg->wii_vc_emulation) return;
        m_context->m_core_cfg->wii_vc_emulation = value;
        wiiVCEmulationChanged();
    }
    bool rcpLagEmulation() { return m_context->m_core_cfg->rcp_lag_emulation; }
    void setRcpLagEmulation(bool value)
    {
        if (value == m_context->m_core_cfg->rcp_lag_emulation) return;
        m_context->m_core_cfg->rcp_lag_emulation = value;
        rcpLagEmulationChanged();
    }
    double cpuCF() { return m_context->m_core_cfg->cpu_cf; }
    void setCpuCF(double value)
    {
        if (value == m_context->m_core_cfg->cpu_cf) return;
        m_context->m_core_cfg->cpu_cf = value;
        cpuCFChanged();
    }
    double rcpLagFactor() { return m_context->m_core_cfg->rcp_lag_factor; }
    void setRcpLagFactor(double value)
    {
        if (value == m_context->m_core_cfg->rcp_lag_factor) return;
        m_context->m_core_cfg->rcp_lag_factor = value;
        rcpLagFactorChanged();
    }
    bool floatExceptionEmulation() { return m_context->m_core_cfg->float_exception_emulation; }
    void setFloatExceptionEmulation(bool value)
    {
        if (value == m_context->m_core_cfg->float_exception_emulation) return;
        m_context->m_core_cfg->float_exception_emulation = value;
        floatExceptionEmulationChanged();
    }
    bool useSummercart() { return m_context->m_core_cfg->use_summercart; }
    void setUseSummercart(bool value)
    {
        if (value == m_context->m_core_cfg->use_summercart) return;
        m_context->m_core_cfg->use_summercart = value;
        useSummercartChanged();
    }
    bool stScreenshot() { return m_context->m_core_cfg->st_screenshot; }
    void setStScreenshot(bool value)
    {
        if (value == m_context->m_core_cfg->st_screenshot) return;
        m_context->m_core_cfg->st_screenshot = value;
        stScreenshotChanged();
    }
    bool stLZ4() { return m_context->m_core_cfg->st_lz4; }
    void setStLz4(bool value)
    {
        if (value == m_context->m_core_cfg->st_lz4) return;
        m_context->m_core_cfg->st_lz4 = value;
        stLZ4Changed();
    }
    int romCacheSize() { return m_context->m_core_cfg->rom_cache_size; }
    void setRomCacheSize(int value)
    {
        if (value == m_context->m_core_cfg->rom_cache_size) return;
        m_context->m_core_cfg->rom_cache_size = value;
        romCacheSizeChanged();
    }
    bool audioDelayEnabled() { return m_context->m_core_cfg->is_audio_delay_enabled; }
    void setAudioDelayEnabled(bool value)
    {
        if (value == m_context->m_core_cfg->is_audio_delay_enabled) return;
        m_context->m_core_cfg->is_audio_delay_enabled = value;
        audioDelayEnabledChanged();
    }
    bool compiledJumpEnabled() { return m_context->m_core_cfg->is_compiled_jump_enabled; }
    void setCompiledJumpEnabled(bool value)
    {
        if (value == m_context->m_core_cfg->is_compiled_jump_enabled) return;
        m_context->m_core_cfg->is_compiled_jump_enabled = value;
        compiledJumpEnabledChanged();
    }
    bool ceqsNaNAccurate() { return m_context->m_core_cfg->c_eq_s_nan_accurate; }
    void setCeqsNaNAccurate(bool value)
    {
        if (value == m_context->m_core_cfg->c_eq_s_nan_accurate) return;
        m_context->m_core_cfg->c_eq_s_nan_accurate = value;
        ceqsNaNAccurateChanged();
    }
    bool accurateRDPCompletion() { return m_context->m_core_cfg->accurate_rdp_completion; }
    void setAccurateRdpCompletion(bool value)
    {
        if (value == m_context->m_core_cfg->accurate_rdp_completion) return;
        m_context->m_core_cfg->accurate_rdp_completion = value;
        accurateRDPCompletionChanged();
    }
    bool vcrBackups() { return m_context->m_core_cfg->vcr_backups; }
    void setVcrBackups(bool value)
    {
        if (value == m_context->m_core_cfg->vcr_backups) return;
        m_context->m_core_cfg->vcr_backups = value;
        vcrBackupsChanged();
    }
    bool vcrWriteExtendedFormat() { return m_context->m_core_cfg->vcr_write_extended_format; }
    void setVcrWriteExtendedFormat(bool value)
    {
        if (value == m_context->m_core_cfg->vcr_write_extended_format) return;
        m_context->m_core_cfg->vcr_write_extended_format = value;
        vcrWriteExtendedFormatChanged();
    }
  signals:
    int coreTypeChanged();
    bool stUndoLoadChanged();
    int maxLagChanged();
    bool wiiVCEmulationChanged();
    bool rcpLagEmulationChanged();
    double cpuCFChanged();
    double rcpLagFactorChanged();
    bool floatExceptionEmulationChanged();
    bool useSummercartChanged();
    bool stScreenshotChanged();
    bool stLZ4Changed();
    int romCacheSizeChanged();
    bool audioDelayEnabledChanged();
    bool compiledJumpEnabledChanged();
    bool ceqsNaNAccurateChanged();
    bool accurateRDPCompletionChanged();
    bool vcrBackupsChanged();
    bool vcrWriteExtendedFormatChanged();
    bool vcrResetRecordingEnabledChanged();

  private:
    EmuContext *m_context;
};

class EmuPaths : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString romDir READ romDir WRITE setRomDir NOTIFY romDirChanged)
    Q_PROPERTY(QString saveDir READ saveDir WRITE setSaveDir NOTIFY saveDirChanged)
    Q_PROPERTY(QString screenshotDir READ screenshotDir WRITE setScreenshotDir NOTIFY screenshotDirChanged)
    Q_PROPERTY(QString backupDir READ backupDir WRITE setBackupDir NOTIFY backupDirChanged)
  public:
    EmuPaths(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~EmuPaths() {}

    QString romDir() const { return QString(m_rom_dir.u16string()); }
    QString saveDir() const { return QString(m_save_dir.u16string()); }
    QString screenshotDir() const { return QString(m_screenshot_dir.u16string()); }
    QString backupDir() const { return QString(m_backup_dir.u16string()); }

    std::filesystem::path romDirStdPath() const { return m_rom_dir; }
    std::filesystem::path saveDirStdPath() const { return m_save_dir; }
    std::filesystem::path screenshotDirStdPath() const { return m_screenshot_dir; }
    std::filesystem::path backupDirStdPath() const { return m_backup_dir; }

    void setRomDir(const QString &value)
    {
        if (value.toStdU16String() == m_rom_dir) return;
        m_rom_dir = value.toStdU16String();
        romDirChanged();
    }
    void setSaveDir(const QString &value)
    {
        if (value.toStdU16String() == m_save_dir) return;
        m_save_dir = value.toStdU16String();
        saveDirChanged();
    }
    void setScreenshotDir(const QString &value)
    {
        if (value.toStdU16String() == m_screenshot_dir) return;
        m_screenshot_dir = value.toStdU16String();
        screenshotDirChanged();
    }
    void setBackupDir(const QString &value)
    {
        if (value.toStdU16String() == m_backup_dir) return;
        m_backup_dir = value.toStdU16String();
        backupDirChanged();
    }
  signals:
    void romDirChanged();
    void saveDirChanged();
    void screenshotDirChanged();
    void backupDirChanged();

  private:
    std::filesystem::path m_rom_dir;
    std::filesystem::path m_save_dir;
    std::filesystem::path m_screenshot_dir;
    std::filesystem::path m_backup_dir;
};

namespace CoreUtil
{
void clear_plugin_funcs(core_params &params);
}
