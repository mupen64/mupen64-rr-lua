/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <utility>

/**
 * \brief A module that provides a thread-safe pub/sub messenger.
 */
namespace Messenger
{
namespace detail
{
struct MessageKey
{
    std::type_index type;
    uint64_t value;

    bool operator==(const MessageKey &) const = default;
};

struct MessageKeyHash
{
    size_t operator()(const MessageKey &key) const;
};

void broadcast_impl(MessageKey key, std::any data);
std::function<void()> subscribe_impl(MessageKey key, std::function<void(std::any)> callback);
template <typename MessageT> MessageKey make_key(MessageT message)
{
    static_assert(std::is_enum_v<MessageT>, "Messages must be defined as enum class types.");
    return {std::type_index(typeid(MessageT)), static_cast<uint64_t>(std::to_underlying(message))};
}
} // namespace detail
} // namespace Messenger
