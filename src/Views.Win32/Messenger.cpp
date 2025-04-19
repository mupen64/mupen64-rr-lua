/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Messenger.h>
#include <Main.h>

namespace Messenger
{
#ifdef _DEBUG
#define ASSERT_NOT_CHANGING assert(!g_changing)
#else
#define ASSERT_NOT_CHANGING
#endif

    // Represents a subscriber to a message.
    struct Subscriber {
        // A unique identifier.
        size_t uid;

        // The callback function.
        t_user_callback cb;
    };

    std::vector<std::pair<Message, Subscriber>> g_subscribers;

    std::unordered_map<Message, std::vector<t_user_callback>> g_subscriber_cache;

    // UID accumulator for generating unique subscriber IDs. Only write operation is increment.
    size_t g_uid_accumulator;

    // Whether a message is currently being broadcasted. Used to wait when subscribing.
    std::atomic g_broadcasting = 0;

    // Whether a subscription is currently happening. Used to wait when broadcasting.
    std::atomic g_subscribing = 0;

    void wait_for_broadcast_end()
    {
        while (g_broadcasting != 0)
        {
            g_view_logger->info("[Messenger] Waiting for broadcast end...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void wait_for_subscribe_end()
    {
        while (g_subscribing != 0)
        {
            g_view_logger->info("[Messenger] Waiting for subscribe end...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void init()
    {
    }

    /**
     * Rebuilds the subscriber cache from the current subscriber vector.
     */
    void rebuild_subscriber_cache()
    {
        g_subscriber_cache.clear();

        for (const auto& [key, func] : g_subscribers)
        {
            g_subscriber_cache[key].push_back(func.cb);
        }
    }

    void broadcast(const Message message, std::any data)
    {
        wait_for_subscribe_end();

        ++g_broadcasting;

        for (const auto& subscriber : g_subscriber_cache[message])
        {
            subscriber(data);
        }

        --g_broadcasting;
    }

    std::function<void()> subscribe(Message message, t_user_callback callback)
    {
        wait_for_broadcast_end();
        wait_for_subscribe_end();

        ++g_subscribing;

        Subscriber subscriber = {g_uid_accumulator++, callback};

        g_subscribers.emplace_back(message, subscriber);
        rebuild_subscriber_cache();

        --g_subscribing;

        return [=] {
            wait_for_broadcast_end();
            wait_for_subscribe_end();

            ++g_subscribing;

            std::erase_if(g_subscribers, [=](const auto& pair) {
                return pair.second.uid == subscriber.uid;
            });
            rebuild_subscriber_cache();

            --g_subscribing;
        };
    }
} // namespace Messenger
