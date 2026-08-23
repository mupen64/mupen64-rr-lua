/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QLatin1StringView>
#include <QJSEngine>
#include <QJSValue>
#include <qqmlintegration.h>

#include "QmlCallableContext.hpp"

namespace QJSFunctions
{
namespace details
{

// JS TRAMPOLINE GENERATION
// =========================================================================

// Constexpr version of std::to_string for size_t.
constexpr std::string to_string_dyn(size_t n)
{
    std::string result;
    do
    {
        result.push_back(char('0' + (n % 10)));
        n /= 10;
    } while (n != 0);
    std::reverse(result.begin(), result.end());
    return result;
}

// Generates a comma-separated list of arguments, like
// "arg0, arg1, arg2" etc.
constexpr std::string gen_args_list_dyn(size_t n)
{
    std::string result;
    for (size_t i = 0; i < n; i++)
    {
        if (i > 0) result += ", ";
        result += "arg";
        result += to_string_dyn(i);
    }

    return result;
}

// Generates the JS-side trampoline for n-variable objects.
constexpr std::string gen_js_trampoline_dyn(size_t n)
{
    using namespace std::literals;
    auto args_list = gen_args_list_dyn(n);
    return std::string("c => ((") + args_list + ") => c.call([" + args_list + "]))";
}

// Constexpr-friendly wrapper for the dynamic trampoline generator.
template <size_t N> constexpr std::string_view js_trampoline()
{
    static constexpr size_t len = gen_js_trampoline_dyn(N).size();
    static constexpr std::array<char, len> data = [] {
        std::array<char, len> tmp;
        std::ranges::copy(gen_js_trampoline_dyn(N), tmp.begin());
        return tmp;
    }();
    return std::string_view(data.data(), len);
}

// Test cases.
static_assert(js_trampoline<0>() == "c => (() => c.call([]))");
static_assert(js_trampoline<1>() == "c => ((arg0) => c.call([arg0]))");
static_assert(js_trampoline<2>() == "c => ((arg0, arg1) => c.call([arg0, arg1]))");

// C++ TRAMPOLINE GENERATION
// =========================================================================

// Like integer_sequence but for types.
template <class... Ts> struct type_sequence
{
    static constexpr size_t size = sizeof...(Ts);
};

// Helper type to extract parameters and return types from member functions.
template <class F> struct member_function_traits;

template <class R, class C, class... Args> struct member_function_traits<R (C::*)(Args...)>
{
    using params = type_sequence<Args...>;
    using result = R;
};

template <class R, class C, class... Args> struct member_function_traits<R (C::*)(Args...) const>
{
    using params = type_sequence<Args...>;
    using result = R;
};

// Helper type to extract parameters and return types from function pointers and simple functors.
template <class F> struct callable_traits
{
    // default: assume type is a functor
    using params = typename member_function_traits<decltype(&F::operator())>::params;
    using result = typename member_function_traits<decltype(&F::operator())>::result;
};

template <class R, class... Args> struct callable_traits<R(Args...)>
{
    // specialization for function
    using params = type_sequence<Args...>;
    using result = R;
};

template <class R, class... Args> struct callable_traits<R (&)(Args...)>
{
    // specialization for function references
    using params = type_sequence<Args...>;
    using result = R;
};

template <class R, class... Args> struct callable_traits<R (*)(Args...)>
{
    // specialization for function pointers
    using params = type_sequence<Args...>;
    using result = R;
};

// The C++ side of the trampoline from the JS function. Converts values to C++, calls the function, and converts the
// return value back to JS.
template <class F, class R, size_t... Is, class... Ts>
inline QJSValue cpp_trampoline_inner(QJSEngine *engine, const QJSValue &params, F &callable, std::type_identity<R>,
                                     std::index_sequence<Is...>, type_sequence<Ts...>)
{
    auto result = callable(engine->fromScriptValue<Ts>(params.property(Is))...);
    return engine->toScriptValue(result);
}

// The C++ side of the trampoline from the JS function (when returning void). Converts values to C++, calls the
// function, then returns undefined.
template <class F, size_t... Is, class... Ts>
inline QJSValue cpp_trampoline_inner(QJSEngine *engine, const QJSValue &params, F &callable, std::type_identity<void>,
                                     std::index_sequence<Is...>, type_sequence<Ts...>)
{
    callable(engine->fromScriptValue<Ts>(params.property(Is))...);
    return QJSValue::UndefinedValue;
}

// Generates the outer wrapper for the C++ trampoline, which moves the callable as a lambda.
template <class F> auto gen_cpp_trampoline(F &&callable)
{
    static_assert(!std::is_lvalue_reference_v<F>, "Trampoline may only be generated for rvalue references");

    using params_sequence = typename callable_traits<F>::params;
    using param_indices = std::make_index_sequence<params_sequence::size>;
    using result_type = std::type_identity<typename callable_traits<F>::result>;

    return [callable = std::move(callable)](QJSEngine *engine, const QJSValue &params) -> QJSValue {
        return cpp_trampoline_inner(engine, params, callable, result_type{}, param_indices{}, params_sequence{});
    };
}
// Generates the outer wrapper for the C++ trampoline, which references the callable as a lambda.
template <class F> auto gen_cpp_trampoline(F &callable)
{
    using params_sequence = typename callable_traits<F>::params;
    using param_indices = std::make_index_sequence<params_sequence::size>;
    using result_type = std::type_identity<typename callable_traits<F>::result>;

    return [&callable](QJSEngine *engine, const QJSValue &params) -> QJSValue {
        return cpp_trampoline_inner(engine, params, callable, result_type{}, param_indices{}, params_sequence{});
    };
}

} // namespace details

/**
 * @brief Wraps a C++ function pointer or functor into a JS callable.
 * @param engine The JS engine to use.
 * @param callable The callable type to use.
 * @return A `QJSValue` containing the JS function bound to this C++ function.
 */
template <class F> QJSValue to_js_function(QJSEngine *engine, F &&callable)
{
    using params_sequence = typename details::callable_traits<F>::params;
    constexpr size_t params_size = params_sequence::size;

    // Generate inner trampoline (QObject)
    auto js_context = engine->newQObject(new QmlCallableContext(details::gen_cpp_trampoline(callable)));
    // Generate outer trampoline (wrapping function)
    // NOTE: JS side is forced to reparse the wrapper function each time, should this be cached?
    return engine->evaluate(QAnyStringView(details::js_trampoline<params_sequence::size>()).toString())
        .call({js_context});
}
} // namespace QJSFunctions