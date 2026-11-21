/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <format>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class I18n
{
  public:
    /**
     * \threadsafety This function is thread-safe.
     * \return The translation registry.
     */
    static I18n &get();

    I18n(const I18n &) = delete;
    I18n &operator=(const I18n &) = delete;
    I18n(I18n &&) = delete;
    I18n &operator=(I18n &&) = delete;

    /**
     * \brief Registers or replaces a translation for a locale.
     * \threadsafety This function is thread-safe.
     * \param key The translation key.
     * \param value The translated value.
     * \param locale The locale associated with the translation.
     */
    void add(std::string key, std::string value, std::string locale);

    /**
     * \brief Sets the locale used by translation lookups.
     * \threadsafety This function is thread-safe.
     * \param locale The locale to use for subsequent lookups.
     */
    void set_locale(std::string locale);

    /**
     * \brief Returns a translation for the current locale.
     * \threadsafety This function is thread-safe.
     * \param key The translation key.
     * \return The translated value, or the key when no translation is registered.
     */
    std::string get(std::string_view key) const;

    /**
     * \brief Returns a translation for a specified locale.
     * \threadsafety This function is thread-safe.
     * \param key The translation key.
     * \param locale The locale to use for the lookup.
     * \return The translated value, or the key when no translation is registered.
     */
    std::string get(std::string_view key, std::string_view locale) const;

    /**
     * \brief Formats a translation from the current locale, returning the raw key if missing.
     * \threadsafety This function is thread-safe.
     * \param key The translation key.
     * \param args Values substituted into the translated format string.
     * \return The formatted translation.
     * \throws std::format_error If the translated format string is invalid for the arguments.
     */
    template <typename... Args> std::string format(std::string_view key, Args &&...args) const
    {
        const auto translation = get(key);
        if (translation == key) return translation;
        return std::vformat(translation, std::make_format_args(args...));
    }

    /**
     * \brief Checks whether a translation exists for the current locale.
     * \threadsafety This function is thread-safe.
     * \param key The translation key.
     * \return true when a translation is registered for the key; otherwise false.
     */
    bool has(std::string_view key) const;

  private:
    I18n() = default;

    // The caller must hold m_mutex.
    std::string get_locked(std::string_view key, std::string_view locale) const;

    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_translations;
    std::string m_locale;
};
