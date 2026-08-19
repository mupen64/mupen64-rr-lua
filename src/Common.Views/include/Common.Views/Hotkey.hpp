/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <variant>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <nlohmann/json.hpp>

/**
 * \brief Represents a combination of a key and modifiers.
 */
struct Hotkey
{
    template <typename T, typename Tag> struct StrongType
    {
        explicit StrongType(T v) : value(v) {}
        explicit operator T() const { return value; }
        T get() const { return value; }

        bool operator==(StrongType other) const { return value == other.value; }
        bool operator!=(StrongType other) const { return value != other.value; }

        bool operator==(T) const = delete;
        bool operator!=(T) const = delete;
        friend bool operator==(T, StrongType) = delete;
        friend bool operator!=(T, StrongType) = delete;

      private:
        T value;
    };

    using KeyCode = StrongType<SDL_Keycode, struct KeyCodeTag>;
    using MouseButton = StrongType<SDL_MouseButtonFlags, struct MouseButtonTag>;
    using Trigger = std::variant<std::monostate, KeyCode, MouseButton>;

    Trigger trigger = std::monostate{};
    bool ctrl{};
    bool shift{};
    bool alt{};

    explicit Hotkey(const Trigger trigger, const bool ctrl = false, const bool shift = false, const bool alt = false)
        : trigger(trigger), ctrl(ctrl), shift(shift), alt(alt)
    {
    }

    Hotkey() = default;

    /**
     * \brief Gets whether the hotkey has an assignment.
     */
    [[nodiscard]] constexpr bool is_assigned() const { return !std::holds_alternative<std::monostate>(trigger); }

    /**
     * \brief Gets whether the hotkey's trigger is effectively empty.
     */
    [[nodiscard]] constexpr bool is_empty() const
    {
        if (!is_assigned()) return false;

        if (ctrl || shift || alt) return false;

        if (std::holds_alternative<KeyCode>(trigger) && std::get<KeyCode>(trigger).get() == SDLK_UNKNOWN) return true;
        if (std::holds_alternative<MouseButton>(trigger) && std::get<MouseButton>(trigger).get() == 0) return true;

        return false;
    }

    /**
     * \brief Gets the string representation of the hotkey.
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * \returns An unassigned hotkey.
     */
    [[nodiscard]] static Hotkey make_unassigned() { return {}; }

    /**
     * \returns A hotkey holding an empty key trigger
     */
    [[nodiscard]] static Hotkey make_empty()
    {
        Hotkey hotkey;
        hotkey.trigger = KeyCode(SDLK_UNKNOWN);
        return hotkey;
    }

    bool operator==(const Hotkey &other) const
    {
        return trigger == other.trigger && ctrl == other.ctrl && shift == other.shift && alt == other.alt;
    }
};

template <typename T, typename Tag> inline void to_json(nlohmann::json &j, const Hotkey::StrongType<T, Tag> &key)
{
    j = key.get();
}

template <typename T, typename Tag> inline void from_json(const nlohmann::json &j, Hotkey::StrongType<T, Tag> &key)
{
    T raw_value;
    j.get_to(raw_value);
    key = Hotkey::StrongType<T, Tag>{raw_value};
}

inline void to_json(nlohmann::json &j, const Hotkey::Trigger &trigger)
{
    std::visit(
        [&j](const auto &v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                j = {{"type", "none"}};
            }
            else if constexpr (std::is_same_v<T, Hotkey::KeyCode>)
            {
                j = {{"type", "keycode"}, {"value", v}};
            }
            else if constexpr (std::is_same_v<T, Hotkey::MouseButton>)
            {
                j = {{"type", "mousebutton"}, {"value", v}};
            }
            else
            {
                throw std::runtime_error("Unhandled Trigger alternative");
            }
        },
        trigger);
}

inline void from_json(const nlohmann::json &j, Hotkey::Trigger &trigger)
{
    std::string type = j.at("type").get<std::string>();
    if (type == "none")
    {
        trigger = std::monostate{};
    }
    else if (type == "keycode")
    {
        Hotkey::KeyCode kc(0);
        j.at("value").get_to(kc);
        trigger = kc;
    }
    else if (type == "mousebutton")
    {
        Hotkey::MouseButton mb(0);
        j.at("value").get_to(mb);
        trigger = mb;
    }
    else
    {
        throw std::runtime_error("Unknown Trigger type: " + type);
    }
}
