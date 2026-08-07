/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Messenger.hpp>
#include <thread>

namespace Messenger
{
using AnyCallback = std::function<void(std::any)>;

// Represents a subscriber to a message.
struct Subscriber
{
    // A unique identifier.
    size_t uid;

    // The callback function.
    AnyCallback cb;
};

struct Context
{
    std::vector<std::pair<Message, Subscriber>> subscribers;

    std::unordered_map<Message, std::vector<AnyCallback>> subscriber_cache;

    // UID accumulator for generating unique subscriber IDs. Only write operation is increment.
    size_t uid_accumulator{};

    // Whether a message is currently being broadcasted. Used to wait when subscribing.
    std::atomic<int32_t> broadcasting{};

    // Whether a subscription is currently happening. Used to wait when broadcasting.
    std::atomic<int32_t> subscribing{};
};

static Context s_ctx;

void wait_for_broadcast_end()
{
    while (s_ctx.broadcasting > 0) std::this_thread::yield();
}

void wait_for_subscribe_end()
{
    while (s_ctx.subscribing > 0) std::this_thread::yield();
}

/**
 * Rebuilds the subscriber cache from the current subscriber vector.
 */
void rebuild_subscriber_cache()
{
    s_ctx.subscriber_cache.clear();

    for (const auto &[key, func] : s_ctx.subscribers)
    {
        s_ctx.subscriber_cache[key].push_back(func.cb);
    }
}

namespace detail
{
void broadcast_impl(const Message message, std::any data)
{
    wait_for_subscribe_end();

    ++s_ctx.broadcasting;

    for (const auto &subscriber : s_ctx.subscriber_cache[message])
    {
        subscriber(data);
    }

    --s_ctx.broadcasting;
}

std::function<void()> subscribe_impl(Message message, AnyCallback callback)
{
    wait_for_broadcast_end();
    wait_for_subscribe_end();

    ++s_ctx.subscribing;

    Subscriber subscriber = {s_ctx.uid_accumulator++, std::move(callback)};

    s_ctx.subscribers.emplace_back(message, subscriber);
    rebuild_subscriber_cache();

    --s_ctx.subscribing;

    return [=] {
        wait_for_broadcast_end();
        wait_for_subscribe_end();

        ++s_ctx.subscribing;

        std::erase_if(s_ctx.subscribers, [=](const auto &pair) { return pair.second.uid == subscriber.uid; });
        rebuild_subscriber_cache();

        --s_ctx.subscribing;
    };
}
} // namespace detail
} // namespace Messenger
