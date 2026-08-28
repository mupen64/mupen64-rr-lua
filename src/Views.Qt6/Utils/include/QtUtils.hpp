/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <future>
#include <Common/MiscHelpers.hpp>
#include <QObject>

namespace QtUtils
{
namespace details
{
template <class T> struct SignalFnTraits
{
    static constexpr bool IS_VALID = false;
};

template <class C> struct SignalFnTraits<void (C::*)()>
{
    static constexpr bool IS_VALID = true;
    using Class = C;
    using Return = void;
};

template <class C, class Arg0> struct SignalFnTraits<void (C::*)(Arg0)>
{
    static constexpr bool IS_VALID = true;
    using Class = C;
    using Return = Arg0;
};

template <class C, class Arg0> struct SignalFnTraits<void (C::*)(const Arg0 &)>
{
    static constexpr bool IS_VALID = true;
    using Class = C;
    using Return = Arg0;
};
} // namespace details

template <class F>
std::future<typename details::SignalFnTraits<F>::Return> on_signal(
    typename details::SignalFnTraits<F>::Class *object, F &&member)
{
    using Return = typename details::SignalFnTraits<F>::Return;

    std::promise<Return> promise;
    auto future = promise.get_future();

    // connect a single-shot listener to resolve the promise
    QObject::connect(
        object, member, object, [promise = std::move(promise)](Return param) mutable { promise.set_value(param); },
        (Qt::ConnectionType)(Qt::AutoConnection | Qt::SingleShotConnection));

    return future;
}

template <class F>
std::future<void> on_signal(typename details::SignalFnTraits<F>::Class *object, F &&member)
    requires(std::is_void_v<typename details::SignalFnTraits<F>::Return>)
{
    std::promise<void> promise;
    auto future = promise.get_future();

    // connect a single-shot listener to resolve the promise
    QObject::connect(
        object, member, object, [promise = std::move(promise)] mutable { promise.set_value(); },
        (Qt::ConnectionType)(Qt::AutoConnection | Qt::SingleShotConnection));

    return future;
}
} // namespace QtUtils