#pragma once

#include <span>
#include <string>

/**
 * \brief Limits a value to a specific range
 * \param value The value to limit
 * \param min The lower bound
 * \param max The upper bound
 * \return The value, limited to the specified range
 */
template <typename T>
static T clamp(const T value, const T min, const T max)
{
    if (value > max)
    {
        return max;
    }
    if (value < min)
    {
        return min;
    }
    return value;
}

/**
 * \brief Formats a duration into a string of format HH:MM:SS
 * \param seconds The duration in seconds
 * \return The formatted duration
 */
static std::wstring format_duration(size_t seconds)
{
    wchar_t str[480] = {};
    wsprintfW(str, L"%02u:%02u:%02u", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
    return str;
}
