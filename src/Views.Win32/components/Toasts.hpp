/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module providing toast notifications.
 */
namespace Toasts
{
/**
 * \brief Represents the data for a toast notification.
 */
struct ToastData
{
    std::string content;
    std::optional<std::string> title = std::nullopt;
    core_dialog_type tone = fsvc_warning;
    std::chrono::milliseconds ttl = std::chrono::seconds(5);
};

/**
 * \brief Shows a toast notification via the appropriate method.
 * \param data The toast data.
 */
void show(const ToastData &data);

/**
 * \brief Updates the layout of visible toasts.
 */
void relayout();
} // namespace Toasts
