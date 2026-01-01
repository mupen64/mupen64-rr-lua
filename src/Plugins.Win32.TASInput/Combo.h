/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Represents a combo.
 */
struct t_combo {
    /**
     * \brief The combo's name.
     */
    std::string name{};

    /**
     * \brief The combo's samples.
     */
    std::vector<core_buttons> samples{};

    /**
     * \return Whether any sample utilizes the joystick (magnitude > 0).
     */
    [[nodiscard]] bool uses_joystick() const;

    /**
     * \brief Serializes the combo to a byte array.
     */
    [[nodiscard]] std::vector<uint8_t> serialize() const;

    /**
     * \brief Deserializes a byte array into a combo.
     * \param data The byte array to deserialize.
     * \return The deserialized combo, or an error message if the data is malformed.
     */
    [[nodiscard]] static std::variant<t_combo, std::wstring> deserialize(const std::span<uint8_t>& data);

    /**
     * \brief Serializes a vector of combos to a byte array.
     * \param combos The combos to serialize.
     * \return The serialized byte array.
     */
    [[nodiscard]] static std::vector<uint8_t> serialize_combos(const std::vector<t_combo>& combos);

    /**
     * \brief Deserializes a byte array into a combo vector.
     * \param data The combos to deserialize.
     * \return The deserialized combos.
     */
    [[nodiscard]] static std::vector<t_combo> deserialize_combos(const std::span<uint8_t>& data);
};
