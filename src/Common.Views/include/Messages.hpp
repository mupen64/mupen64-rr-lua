/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <Config.hpp>
#include <m64rr/Types.hpp>
#include <Messenger.hpp>

namespace Messenger
{
/**
 * \brief Types of messages
 */
enum class Message
{
    /**
     * \brief Debug message used for benchmarking which should not be subscribed to.
     */
    None,

    /**
     * \brief The emulator launched state has changed
     */
    EmuLaunchedChanged,

    /**
     * \brief The core executing state has changed
     */
    CoreExecutingChanged,

    /**
     * \brief The emulator is beginning the termination process
     */
    EmuStopping,

    /**
     * \brief The emulator paused state has changed
     */
    EmuPausedChanged,

    /**
     * \brief The video capturing state has changed
     */
    CapturingChanged,

    /**
     * \brief The statusbar visibility has changed
     */
    StatusbarVisibilityChanged,

    /**
     * \brief The main window size changed
     */
    SizeChanged,

    /**
     * \brief The main window moved.
     */
    MainWindowMoved,

    /**
     * \brief The movie loop state changed
     */
    MovieLoopChanged,

    /**
     * \brief The VCR read-only state changed
     */
    ReadonlyChanged,

    /**
     * \brief The VCR task changed
     */
    TaskChanged,

    /**
     * \brief The current VCR sample index changed
     */
    CurrentSampleChanged,

    /**
     * \brief A VCR unfreeze operation has completed
     */
    UnfreezeCompleted,

    /**
     * \brief The VCR warp modify status has changed
     */
    WarpModifyStatusChanged,

    /**
     * \brief The VCR engine has created or destroyed a seek savestate at the specified frame.
     */
    SeekSavestateChanged,

    /**
     * \brief The Lua engine has started a script
     */
    ScriptStarted,

    /**
     * \brief The main window has been created
     */
    AppReady,

    /**
     * \brief The emulator has finished resetting
     */
    ResetCompleted,

    /**
     * \brief The config will begin saving soon
     */
    ConfigSaving,

    /**
     * \brief The config has been loaded and values have changed
     */
    ConfigLoaded,

    ConfigNeedsPatching,

    /**
     * \brief The rerecord count of the currently recorded movie changed
     */
    RerecordsChanged,

    /**
     * \brief The currently selected save slot changed
     */
    SlotChanged,

    /**
     * \brief The multi-frame advance count has changed.
     */
    MultiFrameAdvanceCountChanged,

    /**
     * \brief A VCR seek operation has completed
     */
    SeekCompleted,

    /**
     * \brief The seek status has changed.
     */
    SeekStatusChanged,

    /**
     * \brief The core speed modifier has changed
     */
    SpeedModifierChanged,

    /**
     * \brief The threhsold of VIs since the last input poll has been exceeded
     */
    LagLimitExceeded,

    /**
     * \brief The emu has begun or stopped its starting process
     */
    EmuStartingChanged,

    /**
     * \brief The audio dacrate has changed
     */
    DacrateChanged,

    /**
     * \brief The core fast-forward flag needs updating.
     */
    FastForwardNeedsUpdate,

    ActionRegistryChanged,

    ActionDisplayNameChanged,

    ActionEnabledChanged,

    ActionActiveChanged,
};

template <Message M> struct MessageData;

template <> struct MessageData<Message::EmuLaunchedChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::CoreExecutingChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::EmuStopping>
{
    using type = void;
};
template <> struct MessageData<Message::EmuPausedChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::CapturingChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::StatusbarVisibilityChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::SizeChanged>
{
    using type = std::pair<int32_t, int32_t>;
};
template <> struct MessageData<Message::MainWindowMoved>
{
    using type = void;
};
template <> struct MessageData<Message::MovieLoopChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::ReadonlyChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::TaskChanged>
{
    using type = core_vcr_task;
};
template <> struct MessageData<Message::CurrentSampleChanged>
{
    using type = int32_t;
};
template <> struct MessageData<Message::UnfreezeCompleted>
{
    using type = void;
};
template <> struct MessageData<Message::WarpModifyStatusChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::SeekSavestateChanged>
{
    using type = size_t;
};
template <> struct MessageData<Message::ScriptStarted>
{
    using type = std::filesystem::path;
};
template <> struct MessageData<Message::AppReady>
{
    using type = void;
};
template <> struct MessageData<Message::ResetCompleted>
{
    using type = void;
};
template <> struct MessageData<Message::ConfigSaving>
{
    using type = void;
};
template <> struct MessageData<Message::ConfigLoaded>
{
    using type = void;
};
template <> struct MessageData<Message::ConfigNeedsPatching>
{
    using type = t_config&;
};
template <> struct MessageData<Message::RerecordsChanged>
{
    using type = uint64_t;
};
template <> struct MessageData<Message::SlotChanged>
{
    using type = size_t;
};
template <> struct MessageData<Message::MultiFrameAdvanceCountChanged>
{
    using type = void;
};
template <> struct MessageData<Message::SeekCompleted>
{
    using type = void;
};
template <> struct MessageData<Message::SeekStatusChanged>
{
    using type = void;
};
template <> struct MessageData<Message::SpeedModifierChanged>
{
    using type = int32_t;
};
template <> struct MessageData<Message::LagLimitExceeded>
{
    using type = void;
};
template <> struct MessageData<Message::EmuStartingChanged>
{
    using type = bool;
};
template <> struct MessageData<Message::DacrateChanged>
{
    using type = CoreSystemType;
};
template <> struct MessageData<Message::FastForwardNeedsUpdate>
{
    using type = void;
};
template <> struct MessageData<Message::ActionRegistryChanged>
{
    using type = void;
};
template <> struct MessageData<Message::ActionDisplayNameChanged>
{
    using type = std::vector<std::wstring>;
};
template <> struct MessageData<Message::ActionEnabledChanged>
{
    using type = std::vector<std::wstring>;
};
template <> struct MessageData<Message::ActionActiveChanged>
{
    using type = std::vector<std::wstring>;
};

/**
 * \brief Broadcasts a message to all listeners
 * \tparam M The message type
 * \param data The message data
 * \remark This method is thread-safe.
 */
template <Message M> void broadcast(typename MessageData<M>::type data)
{
    static_assert(!std::is_void_v<typename MessageData<M>::type>,
                  "This message does not carry data; call broadcast<M>() instead of broadcast<M>(data).");
    if constexpr (std::is_reference_v<typename MessageData<M>::type>)
    {
        // std::any cannot hold references, so deliver the address of the original object rather than moving from it.
        detail::broadcast_impl(detail::make_key(M), std::any(&data));
    }
    else
    {
        detail::broadcast_impl(detail::make_key(M), std::any(std::move(data)));
    }
}

/**
 * \brief Broadcasts a message which carries no data to all listeners
 * \tparam M The message type
 * \remark This method is thread-safe.
 */
template <Message M> void broadcast()
{
    static_assert(std::is_void_v<typename MessageData<M>::type>,
                  "This message carries data; call broadcast<M>(data) instead of broadcast<M>().");
    detail::broadcast_impl(detail::make_key(M), std::any{});
}

/**
 * \brief Subscribe to a message
 * \tparam M The message type to listen for
 * \param callback The callback to be invoked upon receiving the specified message type
 * \return A function which, when called, unsubscribes from the message
 * \remark This method is thread-safe.
 */
template <Message M, typename F> std::function<void()> subscribe(F callback)
{
    if constexpr (std::is_void_v<typename MessageData<M>::type>)
    {
        static_assert(std::is_invocable_v<F>, "The callback for this message must be callable with no arguments.");
        return detail::subscribe_impl(detail::make_key(M), [cb = std::move(callback)](std::any) { std::invoke(cb); });
    }
    else
    {
        using type = typename MessageData<M>::type;
        static_assert(std::is_invocable_v<F, type>,
                      "The callback for this message must be callable with its data type.");
        return detail::subscribe_impl(detail::make_key(M), [cb = std::move(callback)](std::any data) {
            if constexpr (std::is_reference_v<type>)
            {
                using value_type = std::remove_reference_t<type>;
                // Reference payloads are broadcast as a pointer to the original object; dereference to hand the callback its reference.
                std::invoke(cb, *std::any_cast<value_type*>(data));
            }
            else
            {
                std::invoke(cb, std::any_cast<type>(std::move(data)));
            }
        });
    }
}
} // namespace Messenger
