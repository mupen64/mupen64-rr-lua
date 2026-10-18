/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <windows.h>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>

/**
 * Represents a dispatcher that can execute functions on a window's UI thread.
 */
class Dispatcher
{
    struct Invocation
    {
        const std::function<void()> &func;
        std::exception_ptr exception;
        bool completed{};
    };

    HWND m_hwnd;
    DWORD m_thread_id;
    std::mutex m_mutex;
    std::deque<std::function<void()>> m_pending;

  public:
    static constexpr UINT message_id = WM_USER + 18;

    /**
     * Creates a new dispatcher with a window as its target.
     * \param hwnd The window.
     */
    explicit Dispatcher(HWND hwnd);

    /**
     * \brief Executes a function on the window's UI thread synchronously.
     * \param func The function to be executed.
     * \remarks Callback exceptions are rethrown on the calling thread.
     */
    void invoke(const std::function<void()> &func);

    /**
     * \brief Executes a function on the window's UI thread asynchronously.
     * \param func The function to be executed.
     */
    void post(const std::function<void()> &func);

    /**
     * \brief Handles dispatched work. The target window must call this when it receives message_id.
     * \param lparam The message's lParam, or zero to drain asynchronously posted work.
     * \remarks Must be called on the window's UI thread. The window must remain alive while dispatching work.
     */
    void execute(LPARAM lparam = 0);
};
