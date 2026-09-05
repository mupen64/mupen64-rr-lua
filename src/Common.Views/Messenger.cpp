/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/Messenger.hpp>

#include <unordered_map>
#include <utility>

namespace Messenger
{
using AnyCallback = std::function<void(std::any)>;

struct Subscriber
{
    size_t uid;
    AnyCallback cb;
};

struct Context
{
    std::vector<std::pair<detail::MessageKey, Subscriber>> subscribers;

    std::unordered_map<detail::MessageKey, std::vector<AnyCallback>> subscriber_cache;

    // UID accumulator for generating unique subscriber IDs. Only write operation is increment.
    size_t uid_accumulator{};

    // Whether a message is currently being broadcasted. Used to wait when subscribing.
    std::atomic<int32_t> broadcasting;

    // Whether a subscription is currently happening. Used to wait when broadcasting.
    std::atomic<int32_t> subscribing;
};

static Context g_ctx;

} // namespace Messenger

namespace std
{
size_t hash<Messenger::detail::MessageKey>::operator()(const Messenger::detail::MessageKey &key) const noexcept
{
    size_t seed = std::hash<std::type_index>{}(key.type);
    seed ^= std::hash<uint64_t>{}(key.value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
} // namespace std

namespace Messenger
{
void wait_for_broadcast_end()
{
    while (g_ctx.broadcasting > 0) std::this_thread::yield();
}

void wait_for_subscribe_end()
{
    while (g_ctx.subscribing > 0) std::this_thread::yield();
}

/**
 * Rebuilds the subscriber cache from the current subscriber vector.
 */
void rebuild_subscriber_cache()
{
    g_ctx.subscriber_cache.clear();

    for (const auto &[key, subscriber] : g_ctx.subscribers)
    {
        g_ctx.subscriber_cache[key].push_back(subscriber.cb);
    }
}

namespace detail
{
void broadcast_impl(const MessageKey key, std::any data)
{
    wait_for_subscribe_end();

    ++g_ctx.broadcasting;

    for (const auto &subscriber : g_ctx.subscriber_cache[key])
    {
        subscriber(data);
    }

    --g_ctx.broadcasting;
}

std::function<void()> subscribe_impl(MessageKey key, AnyCallback callback)
{
    wait_for_broadcast_end();
    wait_for_subscribe_end();

    ++g_ctx.subscribing;

    Subscriber subscriber = {g_ctx.uid_accumulator++, std::move(callback)};

    g_ctx.subscribers.emplace_back(key, subscriber);
    rebuild_subscriber_cache();

    --g_ctx.subscribing;

    return [=] {
        wait_for_broadcast_end();
        wait_for_subscribe_end();

        ++g_ctx.subscribing;

        std::erase_if(g_ctx.subscribers, [=](const auto &pair) { return pair.second.uid == subscriber.uid; });
        rebuild_subscriber_cache();

        --g_ctx.subscribing;
    };
}
} // namespace detail
} // namespace Messenger
