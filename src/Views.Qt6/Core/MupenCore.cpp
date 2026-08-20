/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MupenCore.hpp"
#include <ViewModels.Qt6/Core.hpp>

#include <atomic>
#include <ranges>


#include <QThread>
#include <QUrl>

static std::atomic_bool g_core_context_created;

CoreContext::CoreContext(QObject *parent) : QObject(parent)
{
    if (g_core_context_created.exchange(true))
        throw std::logic_error("CoreContext should only be created once");
    auto &params = Core::params();

    // Override dialog service
    params.show_multiple_choice_dialog = [&](std::string_view id, const std::vector<std::string> &choices,
                                             const char *str, const char *title, core_dialog_type type) -> size_t {
        auto conn_type = thread()->isCurrentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

        // Convert choices to Qt string
        auto q_choices = choices | std::views::transform(QString::fromStdString) | std::ranges::to<QList>();
        // clang-format off
        return QMetaObject::invokeMethod(
            this, &CoreContext::showMultipleChoiceDialog, conn_type, 
            QAnyStringView(title), QAnyStringView(str), q_choices, CoreDialogType::from_core(type));
        // clang-format on
    };
    params.show_ask_dialog = [&](std::string_view id, const char *str, const char *title, bool warning) -> bool {
        auto conn_type = thread()->isCurrentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
        // clang-format off
        return QMetaObject::invokeMethod(
            this, &CoreContext::showAskDialog, conn_type, 
            QAnyStringView(title), QAnyStringView(str), warning ? CoreDialogType::Warning : CoreDialogType::Information);
        // clang-format on
    };
    params.show_dialog = [&](const char *str, const char *title, core_dialog_type type) {
        auto conn_type = thread()->isCurrentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
        // clang-format off
        QMetaObject::invokeMethod(
            this, &CoreContext::showDialog, conn_type, 
            QAnyStringView(title), QAnyStringView(str), CoreDialogType::from_core(type));
        // clang-format on
    };

    Core::context();
}

CoreContext::~CoreContext()
{
}

CoreResult::Value CoreContext::vrStartROM(const QUrl &url)
{
    return (CoreResult::Value)(int)Core::context()->vr_start_rom(url.toLocalFile().toStdU16String());
}

CoreResult::Value CoreContext::vrCloseROM(bool resetVCR)
{
    return (CoreResult::Value)(int)Core::context()->vr_close_rom(resetVCR);
}