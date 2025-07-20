#pragma once

// Throwaway actions which can be spammed get keys as to not clog up the async executor queue
#define ASYNC_KEY_CLOSE_ROM (1)
#define ASYNC_KEY_START_ROM (2)
#define ASYNC_KEY_RESET_ROM (3)
#define ASYNC_KEY_PLAY_MOVIE (4)

/**
 * \brief A module responsible for implementing standard application actions.
 */
namespace AppActions
{
    /**
     * \brief Adds the standard app actions to the action registry.
     */
    void add();
} // namespace AppActions
