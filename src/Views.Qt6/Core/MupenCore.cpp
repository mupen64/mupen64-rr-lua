/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MupenCore.hpp"
#include <MiscHelpers.hpp>

#include <atomic>
#include <ranges>

#include <QThread>
#include <QIcon>
#include <QUrl>

#include <ViewModels.Qt6/Core.hpp>
#include <ViewModels.Qt6/QtUtils.hpp>

static std::atomic_bool g_core_context_created = false;

CoreContext::CoreContext(QObject *parent) : QObject(parent)
{
    if (g_core_context_created.exchange(true)) throw std::logic_error("CoreContext should only be created once");
    auto &params = Core::params();

    // Override dialog service
    params.show_multiple_choice_dialog = [&](std::string_view id, const std::vector<std::string> &choices,
                                             const char *str, const char *title, core_dialog_type type) -> size_t {
        // Convert choices to Qt string
        auto q_choices = choices | std::views::transform(QString::fromStdString) | std::ranges::to<QList>();

        // Future waiting for GUI result
        // FIXME: if the frontend has a dialog active this may fire early before we show our dialog
        auto dialog_finished = QtUtils::on_signal(this, &CoreContext::showMultipleChoiceDialogFinished);

        // clang-format off
        // signal GUI to show dialog
        return QMetaObject::invokeMethod(
            this, &CoreContext::showMultipleChoiceDialog, Qt::AutoConnection,
            QAnyStringView(title), QAnyStringView(str), q_choices, CoreDialogType::from_core(type));
        // clang-format on

        // wait/acknowledge result
        dialog_finished.wait();
        return dialog_finished.get();
    };
    params.show_ask_dialog = [&](std::string_view id, const char *str, const char *title, bool warning) -> bool {
        // Future waiting for GUI result
        auto dialog_finished = QtUtils::on_signal(this, &CoreContext::showAskDialogFinished);

        // clang-format off
        // signal GUI to show dialog
        QMetaObject::invokeMethod(
            this, &CoreContext::showAskDialog, Qt::AutoConnection,
            QAnyStringView(title), QAnyStringView(str), warning ? CoreDialogType::Warning : CoreDialogType::Information);
        // clang-format on

        // wait/acknowledge result
        dialog_finished.wait();
        return dialog_finished.get();
    };
    params.show_dialog = [&](const char *str, const char *title, core_dialog_type type) {
        // Future waiting for GUI result
        auto dialog_finished = QtUtils::on_signal(this, &CoreContext::showDialogFinished);

        // clang-format off
        QMetaObject::invokeMethod(
            this, &CoreContext::showDialog, Qt::AutoConnection, 
            QAnyStringView(title), QAnyStringView(str), CoreDialogType::from_core(type));
        // clang-format on

        // wait/acknowledge result
        dialog_finished.wait();
        dialog_finished.get();
    };

    Core::context();
}

CoreContext::~CoreContext()
{
    Core::context()->vr_close_rom(true);
}

CoreResult::Value CoreContext::vrStartROM(const QUrl &url)
{
    return (CoreResult::Value)(int)Core::context()->vr_start_rom(url.toLocalFile().toStdU16String());
}

CoreResult::Value CoreContext::vrCloseROM(bool resetVCR)
{
    return (CoreResult::Value)(int)Core::context()->vr_close_rom(resetVCR);
}