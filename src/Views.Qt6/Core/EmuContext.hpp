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

#include <Core/API.hpp>
#include "plugin/Plugin.hpp"

#include "CoreEnums.hpp"

/**
 * @brief QML-owned singleton holding the core and related objects.
 * Implemented as a non-singleton, but will throw a tantrum if instantiated more than once.
 */
class EmuContext : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // core_ctx properties
    Q_PROPERTY(bool launched READ isLaunched NOTIFY launchedChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool coreExecuting READ isCoreExecuting NOTIFY coreExecutingChanged)
    Q_PROPERTY(bool gsButton READ isGSButton WRITE setGSButton NOTIFY gsButtonChanged)

    // core_cfg properties
    Q_PROPERTY(int32_t speedModifier READ speedModifier WRITE setSpeedModifier NOTIFY speedModifierChanged)
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
    CoreCfg *m_core_cfg;
    CoreParams *m_core_params;
    core_ctx *m_core_ctx;

    std::optional<PluginSet> m_plugins;
    M64RRSpec::PtrReadVideo m_fn_read_video;

    QThreadPool m_task_pool;
};

namespace CoreUtil
{
void clear_plugin_funcs(CoreParams &params);
}
