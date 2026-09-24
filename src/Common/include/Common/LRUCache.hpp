/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace LRU
{
/**
 * \brief A simple LRU cache with a maximum size and an optional deleter for evicted values.
 */
template <typename K, typename V, typename Hash = std::hash<K>> class Cache
{
  public:
    Cache() = default;

    /**
     * \brief Creates a cache with a maximum size
     * \param size The cache's maximum size
     * \param deleter Deleter function for evicted and cleared values
     */
    Cache(size_t size, std::function<void(V)> deleter) : m_size(size), m_deleter(std::move(deleter)) {}

    Cache(const Cache &) = delete;
    Cache &operator=(const Cache &) = delete;
    Cache(Cache &&) = default;
    Cache &operator=(Cache &&) = default;

    ~Cache() { clear(); }

    /**
     * \brief Adds a value to the cache, evicting the least recently used value if the cache is full
     * \param key The key
     * \param value The value
     */
    void add(const K &key, V value)
    {
        if (auto it = m_map.find(key); it != m_map.end())
        {
            if (m_deleter) m_deleter(it->second->second);
            it->second->second = std::move(value);
            touch(it);
            return;
        }

        if (m_size == 0) return;

        if (m_map.size() >= m_size)
        {
            evict_lru();
        }

        m_list.emplace_front(key, std::move(value));
        m_map.emplace(key, m_list.begin());
    }

    /**
     * \brief Removes all elements from the cache, calling the deleter for each value
     */
    void clear()
    {
        if (m_deleter)
        {
            for (auto &[key, value] : m_list)
            {
                m_deleter(value);
            }
        }
        m_map.clear();
        m_list.clear();
    }

    /**
     * \brief Gets a value from the cache via a key, marking it as most recently used
     * \param key The key
     * \return The value associated with the key, or nothing if the key does not exist
     */
    std::optional<V> get(const K &key)
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return std::nullopt;

        touch(it);
        return it->second->second;
    }

    /** Gets a cached value without copying it, marking it as most recently used. */
    V *get_ref(const K &key)
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return nullptr;

        touch(it);
        return &it->second->second;
    }

    /**
     * \brief Checks if the cache contains a key
     */
    bool contains(const K &key) { return m_map.contains(key); }

    /**
     * \brief Gets the current size of the cache
     */
    size_t size() const { return m_map.size(); }

  private:
    using ListIterator = typename std::list<std::pair<K, V>>::iterator;

    void touch(typename std::unordered_map<K, ListIterator, Hash>::iterator it)
    {
        m_list.splice(m_list.begin(), m_list, it->second);
        it->second = m_list.begin();
    }

    void evict_lru()
    {
        if (m_list.empty()) return;

        auto last = std::prev(m_list.end());
        if (m_deleter) m_deleter(last->second);
        m_map.erase(last->first);
        m_list.erase(last);
    }

    size_t m_size{};
    std::function<void(V)> m_deleter{};
    std::list<std::pair<K, V>> m_list{};
    std::unordered_map<K, ListIterator, Hash> m_map{};
};
} // namespace LRU
