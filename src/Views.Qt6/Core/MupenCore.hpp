/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QFuture>
#include <QUrl>
#include <qqmlintegration.h>

#include <m64rr/API.hpp>
#include "plugin/Plugin.hpp"

#include "CoreEnums.hpp"

/**
 * @brief QML-owned singleton holding the core and related objects.
 */
class CoreContext : public QObject {
    Q_OBJECT
    QML_ELEMENT
  public:
    CoreContext(QObject* parent = nullptr);
    virtual ~CoreContext();

    static CoreContext* instance();

    static core_ctx* raw_context() {
      auto* inst = instance();
      return (inst != nullptr) ? inst->m_core_ctx : nullptr;
    }

    // vr_* functions
    // ==========================

    Q_INVOKABLE CoreResult::Value vrStartROM(const QUrl& url) const;

    Q_INVOKABLE CoreResult::Value vrCloseROM(bool resetVCR = true) const;

    // vr_* properties
    // ==========================

  signals:

    // Dialog service
    // ==========================
    // Dialog closure is notified via a separate signal.

    void showMultipleChoiceDialog(QAnyStringView title, QAnyStringView content, const QList<QString>& choices, CoreDialogType::Value type);
    void showMultipleChoiceDialogFinished(size_t result);

    void showAskDialog(QAnyStringView title, QAnyStringView content, CoreDialogType::Value type);
    void showAskDialogFinished(bool result);

    void showDialog(QAnyStringView title, QAnyStringView content, CoreDialogType::Value type);
    void showDialogFinished();

  private:
    core_cfg* m_core_cfg;
    core_params* m_core_params;
    core_ctx* m_core_ctx;

    std::optional<PluginSet> m_plugins;
};

namespace CoreUtil {
  void clear_plugin_funcs(core_params &params);
}