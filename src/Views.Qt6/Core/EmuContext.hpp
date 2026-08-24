/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QUrl>
#include <qqmlintegration.h>

#include <m64rr/API.hpp>
#include "plugin/Plugin.hpp"
#include "QmlCallableContext.hpp"

#include "CoreEnums.hpp"

/**
 * @brief QML-owned singleton holding the core and related objects.
 */
class EmuContext : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool emuLaunched READ isEmuLaunched NOTIFY emuLaunchedChanged)
  public:
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

    Q_INVOKABLE CoreResult::Value vrStartROM(const QUrl &url) const;

    Q_INVOKABLE CoreResult::Value vrCloseROM(bool resetVCR = true) const;

    Q_INVOKABLE void vrInvalidateVisuals() const;

    // vr_* properties
    // ==========================

    bool isEmuLaunched() const;

    // Misc. functions
    // ==========================

    /**
     * @brief Calls the video plugin's `ReadVideo` function, reading out to an image.
     *
     * @param image
     */
    void readVideoOutput(QImage &image);

  signals:

    // Property changes
    // ============================================

    void emuLaunchedChanged(bool value);

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
};

namespace CoreUtil
{
void clear_plugin_funcs(core_params &params);
}