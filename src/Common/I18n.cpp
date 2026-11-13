/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/I18n.hpp>

#include <mutex>

I18n &I18n::get()
{
    static I18n instance;
    return instance;
}

void I18n::add(std::string key, std::string value, std::string locale)
{
    std::unique_lock lock(m_mutex);
    m_translations[std::move(locale)].insert_or_assign(std::move(key), std::move(value));
}

void I18n::set_locale(std::string locale)
{
    std::unique_lock lock(m_mutex);
    m_locale = std::move(locale);
}

std::string I18n::get(std::string_view key) const
{
    std::shared_lock lock(m_mutex);
    return get_locked(key, m_locale);
}

std::string I18n::get(std::string_view key, std::string_view locale) const
{
    std::shared_lock lock(m_mutex);
    return get_locked(key, locale);
}

std::string I18n::get_locked(std::string_view key, std::string_view locale) const
{
    const auto translations = m_translations.find(std::string(locale));
    if (translations == m_translations.end()) return std::string(key);
    const auto translation = translations->second.find(std::string(key));
    return translation == translations->second.end() ? std::string(key) : translation->second;
}

bool I18n::has(std::string_view key) const
{
    std::shared_lock lock(m_mutex);
    const auto locale = m_translations.find(m_locale);
    return locale != m_translations.end() && locale->second.contains(std::string(key));
}
