/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Messenger.hpp>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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

    std::unordered_map<detail::MessageKey, std::vector<AnyCallback>, detail::MessageKeyHash> subscriber_cache;

    // UID accumulator for generating unique subscriber IDs. Only write operation is increment.
    size_t uid_accumulator{};

    // Whether a message is currently being broadcasted. Used to wait when subscribing.
    std::atomic<int32_t> broadcasting;

    // Whether a subscription is currently happening. Used to wait when broadcasting.
    std::atomic<int32_t> subscribing;
};

static Context s_ctx;

size_t detail::MessageKeyHash::operator()(const MessageKey &key) const
{
    size_t seed = std::hash<std::type_index>{}(key.type);
    seed ^= std::hash<uint64_t>{}(key.value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

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

    for (const auto &[key, subscriber] : s_ctx.subscribers)
    {
        s_ctx.subscriber_cache[key].push_back(subscriber.cb);
    }
}

namespace detail
{
void broadcast_impl(const MessageKey key, std::any data)
{
    wait_for_subscribe_end();

    ++s_ctx.broadcasting;

    for (const auto &subscriber : s_ctx.subscriber_cache[key])
    {
        subscriber(data);
    }

    --s_ctx.broadcasting;
}

std::function<void()> subscribe_impl(MessageKey key, AnyCallback callback)
{
    wait_for_broadcast_end();
    wait_for_subscribe_end();

    ++s_ctx.subscribing;

    Subscriber subscriber = {s_ctx.uid_accumulator++, std::move(callback)};

    s_ctx.subscribers.emplace_back(key, subscriber);
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
