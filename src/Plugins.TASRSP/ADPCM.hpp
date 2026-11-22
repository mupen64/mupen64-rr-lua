/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>

extern uint16_t adpcmtable[0x88];

template <int SampleBits>
static inline __m128i decode_adpcm_residuals(const uint8_t *buffer, int input_base, uint16_t &input_offset, int shift)
{
    std::array<int16_t, 8> residuals;
    constexpr int samples_per_byte = 8 / SampleBits;
    constexpr int sample_mask = (1 << SampleBits) - 1;
    for (int i = 0; i < 8; i += samples_per_byte)
    {
        const uint8_t packed = buffer[(input_base + input_offset++) ^ 3];
        for (int j = 0; j < samples_per_byte; ++j)
        {
            const int sample = (packed >> (8 - SampleBits * (j + 1))) & sample_mask;
            residuals[i + j] = static_cast<int16_t>(sample << (16 - SampleBits));
        }
    }
    return _mm_sra_epi16(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(residuals.data())), _mm_cvtsi32_si128(shift));
}

static inline __m128i filter_adpcm_residuals(__m128i residuals, __m128i book1, __m128i book2, __m128i previous_samples)
{
    const __m128i history =
        _mm_shuffle_epi32(_mm_shufflehi_epi16(previous_samples, _MM_SHUFFLE(2, 3, 0, 1)), _MM_SHUFFLE(3, 3, 3, 3));
    __m128i low = _mm_madd_epi16(_mm_unpacklo_epi16(book1, book2), history);
    __m128i high = _mm_madd_epi16(_mm_unpackhi_epi16(book1, book2), history);

    __m128i coefficients = _mm_or_si128(_mm_slli_si128(book2, 2), _mm_cvtsi32_si128(2048));
    for (int i = 0; i < 4; ++i)
    {
        const __m128i next = _mm_slli_si128(coefficients, 2);
        const __m128i pair = _mm_shuffle_epi32(residuals, 0);
        low = _mm_add_epi32(low, _mm_madd_epi16(_mm_unpacklo_epi16(coefficients, next), pair));
        high = _mm_add_epi32(high, _mm_madd_epi16(_mm_unpackhi_epi16(coefficients, next), pair));
        coefficients = _mm_slli_si128(coefficients, 4);
        residuals = _mm_srli_si128(residuals, 4);
    }

    return _mm_packs_epi32(_mm_srai_epi32(_mm_shuffle_epi32(low, _MM_SHUFFLE(2, 3, 0, 1)), 11),
        _mm_srai_epi32(_mm_shuffle_epi32(high, _MM_SHUFFLE(2, 3, 0, 1)), 11));
}

template <int SampleBits>
static inline void decode_adpcm(uint8_t *buffer, int input_base, uint16_t input_offset, int output_offset, int count,
    uint8_t flags, uint8_t *rdram, uint32_t address, uint32_t loop_address)
{
    auto *out = buffer + output_offset;
    if (flags & 1)
        std::memset(out, 0, 32);
    else
        std::memcpy(out, rdram + (flags & 2 ? loop_address : address), 32);

    __m128i history = _mm_loadu_si128(reinterpret_cast<const __m128i *>(out + 16));
    out += 32;
    for (; count > 0; count -= 32)
    {
        const uint8_t header = buffer[(input_base + input_offset++) ^ 3];
        const int shift = std::max(16 - SampleBits - (header >> 4), 0);
        const auto *book = adpcmtable + (header & 0xf) * 16;
        const __m128i book1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(book));
        const __m128i book2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(book + 8));

        const __m128i first = decode_adpcm_residuals<SampleBits>(buffer, input_base, input_offset, shift);
        const __m128i second = decode_adpcm_residuals<SampleBits>(buffer, input_base, input_offset, shift);

        const __m128i first_samples = filter_adpcm_residuals(first, book1, book2, history);
        const __m128i second_samples = filter_adpcm_residuals(second, book1, book2, first_samples);

        _mm_storeu_si128(reinterpret_cast<__m128i *>(out), first_samples);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(out + 16), second_samples);
        history = second_samples;
        out += 32;
    }
    std::memcpy(rdram + address, out - 32, 32);
}
