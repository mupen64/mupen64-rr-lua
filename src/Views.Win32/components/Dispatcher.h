/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * Represents a dispatcher that can execute functions on a specific thread.
 */
class Dispatcher
{
  public:
    /**
     * Creates a new dispatcher.
     * \param thread_id The dispatcher's target thread id.
     * \param execute_callback The callback responsible for marshalling execution of a function to the target thread.
     */
    explicit Dispatcher(const DWORD thread_id, const std::function<void()> &execute_callback)
        : m_execute_callback(execute_callback), m_thread_id(thread_id)
    {
    }

    /**
     * \brief Executes a function on the dispatcher's thread synchronously.
     * \param func The function to be executed.
     */
    void invoke(const std::function<void()> &func);

    /**
     * \brief Executes the pending functions on the current thread. This should be called from the dispatcher's target
     * thread.
     */
    void execute();

  private:
    std::mutex m_mutex;
    std::function<void()> m_execute_callback;
    std::function<void()> m_func;
    DWORD m_thread_id{};

    uint64_t m_overhead_times[60]{};
    double m_overhead_percentages[60]{};
    size_t m_overhead_index{};
    std::chrono::time_point<std::chrono::steady_clock> m_call_start;
};
