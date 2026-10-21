/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Dispatcher.hpp"

#include <stdexcept>
#include <system_error>
#include <utility>

Dispatcher::Dispatcher(HWND hwnd) : m_hwnd(hwnd), m_thread_id(GetWindowThreadProcessId(hwnd, nullptr))
{
    if (!m_thread_id)
    {
        throw std::system_error(
            static_cast<int>(GetLastError()), std::system_category(), "Failed to get dispatcher window thread");
    }
}

void Dispatcher::invoke(const std::function<void()> &func)
{
    if (GetCurrentThreadId() == m_thread_id)
    {
        func();
        return;
    }

    // SendMessage keeps this stack request alive until the UI thread finishes handling it.
    Invocation invocation{func, {}};
    SetLastError(ERROR_SUCCESS);
    SendMessage(m_hwnd, message_id, 0, reinterpret_cast<LPARAM>(&invocation));
    if (!invocation.completed)
    {
        const auto error = GetLastError();
        throw std::system_error(static_cast<int>(error ? error : ERROR_GEN_FAILURE), std::system_category(),
            "Failed to invoke dispatcher callback");
    }
    if (invocation.exception) std::rethrow_exception(invocation.exception);
}

void Dispatcher::post(const std::function<void()> &func)
{
    std::lock_guard lock(m_mutex);
    m_pending.push_back(func);
    if (!PostMessage(m_hwnd, message_id, 0, 0))
    {
        const auto error = GetLastError();
        m_pending.pop_back();
        throw std::system_error(static_cast<int>(error), std::system_category(), "Failed to post dispatcher message");
    }
}

void Dispatcher::execute(LPARAM lparam)
{
    if (GetCurrentThreadId() != m_thread_id)
    {
        throw std::logic_error("Dispatcher::execute() called from incorrect thread");
    }

    if (lparam)
    {
        auto &invocation = *reinterpret_cast<Invocation *>(lparam);
        try
        {
            invocation.func();
        }
        catch (...)
        {
            invocation.exception = std::current_exception();
        }
        invocation.completed = true;
        return;
    }

    for (;;)
    {
        std::function<void()> func;
        {
            std::lock_guard lock(m_mutex);
            if (m_pending.empty()) return;
            func = std::move(m_pending.front());
            m_pending.pop_front();
        }
        func();
    }
}
