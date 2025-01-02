#pragma once
#include <functional>
#include <intsafe.h>

namespace AsyncExecutor
{
    /**
     * Initializes the async executor
     */
    void init();

    /**
     * Stops the async executor
     */
    void stop();

    /**
     * \brief Executes a function on a background thread
     * \param func The function to be executed
     * \param key The function's key used for deduplication. If not 0, the function
     * will not be queued if another function with the same key is already in the
     * queue.
     */
    void invoke_async(const std::function<void()>& func, size_t key = 0);
} // namespace AsyncExecutor
