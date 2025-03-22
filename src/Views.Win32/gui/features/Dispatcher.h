/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


/**
 * Represents a dispatcher that can execute functions on a specific thread.
 */
class Dispatcher {
public:
    /**
     * Creates a new dispatcher.
     * \param thread_id The dispatcher's target thread id.
     * \param execute_callback The callback that will be called when the queue has to be executed on the target thread.
     */
    explicit Dispatcher(const DWORD thread_id, const std::function<void()>& execute_callback) :
        m_execute_callback(execute_callback), m_thread_id(thread_id)
    {
    }

    /**
     * \brief Executes a function on the dispatcher's thread.
     * \param func The function to be executed
     */
    void invoke(const std::function<void()>& func);

    /**
     * \brief Executes the pending functions on the current thread.
     */
    void execute();

private:
    std::function<void()> m_execute_callback;
    DWORD m_thread_id;
    std::queue<std::function<void()>> m_queue;
    std::mutex m_mutex;
};
