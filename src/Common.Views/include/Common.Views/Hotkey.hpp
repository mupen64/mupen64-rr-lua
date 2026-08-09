/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <SDL3/SDL_keycode.h>
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

    KeyCode key = KeyCode(SDLK_UNKNOWN);
    bool ctrl{};
    bool shift{};
    bool alt{};
    bool assigned{};

    explicit Hotkey(const KeyCode key, const bool ctrl = false, const bool shift = false, const bool alt = false)
        : key(key), ctrl(ctrl), shift(shift), alt(alt), assigned(true)
    {
    }

    Hotkey() = default;

    /**
     * \brief Gets whether the hotkey is empty. This is different to having no assignment, as it means an intentional
     * user override.
     */
    [[nodiscard]] bool is_empty() const;

    /**
     * \brief Gets whether the hotkey has no assignment.
     */
    [[nodiscard]] bool is_assigned() const;

    /**
     * \brief Gets the string representation of the hotkey.
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * \brief Gets the string representation of the hotkey.
     */
    [[nodiscard]] std::wstring to_wstring() const;

    /**
     * \returns An empty hotkey.
     */
    [[nodiscard]] static Hotkey make_empty();

    /**
     * \returns An unassigned hotkey.
     */
    [[nodiscard]] static Hotkey make_unassigned();

    bool operator==(const Hotkey &other) const
    {
        return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt &&
               assigned == other.assigned;
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
