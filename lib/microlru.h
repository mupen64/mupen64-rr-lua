// MicroLRU Aurumaker72 2024

#include <functional>
#include <optional>
#include <vector>

namespace MicroLRU
{
	template <typename K, typename V>
	class Cache
	{
	public:
		Cache(): m_size(0)
		{
		}

		/**
		 * \brief Creates a cache with a maximum size
		 * \param size The cache's maximum size
		 * \param deleter Deleter function for evicted values
		 */
		Cache(int size, std::function<void(V)> deleter)
		{
			m_size = size;
			m_deleter = deleter;
		}

		/**
		 * \brief Adds a value to the cache, evicting the least used value if the cache is full
		 * \param key
		 * \param value
		 * \remarks The value is swapped if the key already exists
		 */
		void add(K key, V value)
		{
			if (m_map.size() >= m_size)
			{
				auto lu_key = least_used_key();
				if (lu_key.has_value())
				{
					m_deleter(m_map[lu_key.value()].second);
					m_map.erase(lu_key.value());
				}
			}

			m_map[key] = std::make_pair(0, value);
		}


		/**
		 * \brief Removes all elements from the cache and calls the deleter
		 */
		void clear()
		{
			for (auto const& [_, val] : m_map)
			{
				m_deleter(val.second);
			}
			m_map.clear();
		}

		/**
		 * \brief Gets a value from the cache via a key
		 * \param key The key
		 * \return The value associated with the key, or nothing if the key does not exist
		 */
		std::optional<V> get(K key)
		{
			if (!contains(key))
			{
				return std::nullopt;
			}

			auto& [usage, value] = m_map[key];
			++usage;
			return std::make_optional(value);
		}

		/**
		 * \brief Checks if the cache contains a key
		 */
		bool contains(K key)
		{
			return m_map.contains(key);
		}

		/**
		 * \brief Gets the current size of the cache
		 */
		size_t size()
		{
			return m_map.size();
		}

	private:
		// Gets the least used key
		std::optional<K> least_used_key()
		{
			if (m_map.size() == 0)
			{
				return std::nullopt;
			}

			K key;
			size_t min = SIZE_MAX;

			for (auto& [k, v] : m_map)
			{
				if (v.first < min)
				{
					min = v.first;
					key = k;
				}
			}

			return std::make_optional(key);
		}

		size_t m_size;

		std::function<void(V)> m_deleter;

		// Map of key to pair of usage count and value
		std::unordered_map<K, std::pair<size_t, V>> m_map;
	};
}
