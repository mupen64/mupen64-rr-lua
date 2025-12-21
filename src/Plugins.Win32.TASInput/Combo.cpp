/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Combo.h>

bool t_combo::uses_joystick() const
{
    return std::any_of(samples.begin(), samples.end(), [](const core_buttons sample) {
        return sample.x != 0 || sample.y != 0;
    });
}

std::vector<uint8_t> t_combo::serialize() const
{
    // 1. Write the name of the combo as a null-terminated string.
    std::vector<uint8_t> buffer{};
    for (const auto& c : this->name)
    {
        buffer.emplace_back(c);
    }
    buffer.emplace_back(0);

    // 2. Write the size of the samples vector.
    uint32_t samples_size = this->samples.size();
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&samples_size), reinterpret_cast<uint8_t*>(&samples_size) + sizeof(samples_size));

    // 3. Write the samples vector.
    const auto input_begin_offset = buffer.size();
    buffer.resize(buffer.size() + sizeof(this->samples[0]) * this->samples.size());
    std::memcpy(buffer.data() + input_begin_offset, this->samples.data(), sizeof(this->samples[0]) * this->samples.size());

    return buffer;
}

std::variant<t_combo, std::wstring> t_combo::deserialize(const std::span<uint8_t>& data)
{
    t_combo result{};

    size_t offset = 0;

    // 1. Read the name, stopping if we're about to overflow
    while (offset < data.size() && data[offset] != 0)
    {
        result.name += static_cast<char>(data[offset]);
        ++offset;
    }

    // Check for null-terminator
    if (offset >= data.size() || data[offset] != 0)
    {
        return L"Malformed name.";
    }
    ++offset;

    // 2. Read the size of the samples vector (uint32_t)
    if (offset + sizeof(uint32_t) > data.size())
    {
        return L"Malformed sample size.";
    }

    uint32_t samples_size = 0;
    std::memcpy(&samples_size, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // 3. Read the samples data
    size_t samples_byte_size = samples_size * sizeof(float);
    if (offset + samples_byte_size > data.size())
    {
        return L"Malformed samples.";
    }

    result.samples.resize(samples_size);
    std::memcpy(result.samples.data(), data.data() + offset, samples_byte_size);

    return result;
}

std::vector<uint8_t> t_combo::serialize_combos(const std::vector<t_combo>& combos)
{
    std::vector<uint8_t> buffer{};
    for (const auto& combo : combos)
    {
        const auto serialized = combo.serialize();
        buffer.insert(buffer.end(), serialized.begin(), serialized.end());
    }
    return buffer;
}

std::vector<t_combo> t_combo::deserialize_combos(const std::span<uint8_t>& data)
{
    std::vector<t_combo> combos{};

    size_t offset = 0;
    while (offset < data.size())
    {
        auto result = deserialize(data.subspan(offset));
        if (std::holds_alternative<std::wstring>(result))
        {
            return {};
        }

        combos.emplace_back(std::get<t_combo>(result));
        offset += combos.back().serialize().size();
    }

    return combos;
}
