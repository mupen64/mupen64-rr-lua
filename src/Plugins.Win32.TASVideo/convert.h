#pragma once

const unsigned char Five2Eight[32] = {
    0,   // 00000 = 00000000
    8,   // 00001 = 00001000
    16,  // 00010 = 00010000
    25,  // 00011 = 00011001
    33,  // 00100 = 00100001
    41,  // 00101 = 00101001
    49,  // 00110 = 00110001
    58,  // 00111 = 00111010
    66,  // 01000 = 01000010
    74,  // 01001 = 01001010
    82,  // 01010 = 01010010
    90,  // 01011 = 01011010
    99,  // 01100 = 01100011
    107, // 01101 = 01101011
    115, // 01110 = 01110011
    123, // 01111 = 01111011
    132, // 10000 = 10000100
    140, // 10001 = 10001100
    148, // 10010 = 10010100
    156, // 10011 = 10011100
    165, // 10100 = 10100101
    173, // 10101 = 10101101
    181, // 10110 = 10110101
    189, // 10111 = 10111101
    197, // 11000 = 11000101
    206, // 11001 = 11001110
    214, // 11010 = 11010110
    222, // 11011 = 11011110
    230, // 11100 = 11100110
    239, // 11101 = 11101111
    247, // 11110 = 11110111
    255  // 11111 = 11111111
};

const unsigned char Four2Eight[16] = {
    0,   // 0000 = 00000000
    17,  // 0001 = 00010001
    34,  // 0010 = 00100010
    51,  // 0011 = 00110011
    68,  // 0100 = 01000100
    85,  // 0101 = 01010101
    102, // 0110 = 01100110
    119, // 0111 = 01110111
    136, // 1000 = 10001000
    153, // 1001 = 10011001
    170, // 1010 = 10101010
    187, // 1011 = 10111011
    204, // 1100 = 11001100
    221, // 1101 = 11011101
    238, // 1110 = 11101110
    255  // 1111 = 11111111
};

const unsigned char Three2Four[8] = {
    0,  // 000 = 0000
    2,  // 001 = 0010
    4,  // 010 = 0100
    6,  // 011 = 0110
    9,  // 100 = 1001
    11, // 101 = 1011
    13, // 110 = 1101
    15, // 111 = 1111
};

const unsigned char Three2Eight[8] = {
    0,   // 000 = 00000000
    36,  // 001 = 00100100
    73,  // 010 = 01001001
    109, // 011 = 01101101
    146, // 100 = 10010010
    182, // 101 = 10110110
    219, // 110 = 11011011
    255, // 111 = 11111111
};
const unsigned char Two2Eight[4] = {
    0,   // 00 = 00000000
    85,  // 01 = 01010101
    170, // 10 = 10101010
    255  // 11 = 11111111
};

const unsigned char One2Four[2] = {
    0,  // 0 = 0000
    15, // 1 = 1111
};

const unsigned char One2Eight[2] = {
    0,   // 0 = 00000000
    255, // 1 = 11111111
};

inline void bswap_4_x32_sse2(__m128i &vec)
{
    __m128i tmp1 = _mm_srli_epi32(vec, 24);
    __m128i tmp2 = _mm_slli_epi32(vec, 24);

    __m128i t1 = _mm_and_si128(_mm_srli_epi32(vec, 8), _mm_set1_epi32(0x0000FF00));
    __m128i t2 = _mm_and_si128(_mm_slli_epi32(vec, 8), _mm_set1_epi32(0x00FF0000));

    vec = _mm_or_si128(tmp2, tmp1);
    vec = _mm_or_si128(vec, t1);
    vec = _mm_or_si128(vec, t2);
}

/**
 * \brief Copies data from a source buffer to a destination buffer while performing a byteswap within 4-byte groups.
 * \param src The source buffer.
 * \param dst The destination buffer.
 * \param num_bytes The number of bytes to copy.
 */
inline void unswap_copy(uint8_t *src, uint8_t *dst, u32 num_bytes)
{
    const uintptr_t src_addr = reinterpret_cast<uintptr_t>(src);
    u32 leading_bytes = src_addr & 3;

    if (leading_bytes != 0)
    {
        leading_bytes = 4 - leading_bytes;
        leading_bytes = min(leading_bytes, num_bytes);

        for (u32 i = 0; i < leading_bytes; ++i) dst[i] = src[3 - i];

        src += leading_bytes;
        dst += leading_bytes;
        num_bytes -= leading_bytes;
    }

    const u32 sse_block_size = (num_bytes / 16) * 16;
    for (u32 i = 0; i < sse_block_size; i += 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
        bswap_4_x32_sse2(data);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), data);
        src += 16;
        dst += 16;
    }

    num_bytes -= sse_block_size;

    for (u32 i = 0; i < num_bytes / 4; ++i)
    {
        uint32_t val{};
        std::memcpy(&val, src, sizeof(uint32_t));
        val = (val >> 24 & 0x000000FF) | (val >> 8) & 0x0000FF00 | (val << 8) & 0x00FF0000 | (val << 24) & 0xFF000000;
        std::memcpy(dst, &val, sizeof(uint32_t));
        src += 4;
        dst += 4;
    }

    const u32 trailing_bytes = num_bytes % 4;
    if (trailing_bytes > 0)
    {
        for (u32 i = 0; i < trailing_bytes; ++i) dst[i] = src[3 - i];
    }
}

static inline u32 swapdword(u32 value)
{
    return ((value >> 24) & 0x000000FFu) | ((value >> 8) & 0x0000FF00u) | ((value << 8) & 0x00FF0000u) |
           ((value << 24) & 0xFF000000u);
}

static inline u16 swapword(u16 value)
{
    return static_cast<u16>(((value >> 8) & 0x00FFu) | ((value << 8) & 0xFF00u));
}

static inline u8 expand5to8(u32 v)
{
    return static_cast<u8>((v << 3) | (v >> 2));
}
static inline u8 expand4to8(u32 v)
{
    return static_cast<u8>((v << 4) | v);
}
static inline u8 expand3to8(u32 v)
{
    return static_cast<u8>((v << 5) | (v << 2) | (v >> 1));
}

inline void DWordInterleave(void *mem, u32 numDWords)
{
    auto base = reinterpret_cast<std::uint8_t *>(mem);
    for (u32 i = 0; i < numDWords; ++i)
    {
        u32 *a = reinterpret_cast<u32 *>(base + i * 8);
        u32 *b = reinterpret_cast<u32 *>(base + i * 8 + 4);
        std::swap(*a, *b);
    }
}

inline void QWordInterleave(void *mem, u32 numDWords)
{
    auto base = reinterpret_cast<std::uint8_t *>(mem);
    u32 iterations = (numDWords >> 1);
    for (u32 i = 0; i < iterations; ++i)
    {
        u32 *a1 = reinterpret_cast<u32 *>(base + i * 16 + 0);
        u32 *b1 = reinterpret_cast<u32 *>(base + i * 16 + 8);
        std::swap(*a1, *b1);

        u32 *a2 = reinterpret_cast<u32 *>(base + i * 16 + 4);
        u32 *b2 = reinterpret_cast<u32 *>(base + i * 16 + 12);
        std::swap(*a2, *b2);
    }
}

inline u16 RGBA8888_RGBA4444(u32 color)
{
    u8 a = static_cast<u8>((color >> 0) & 0xFFu);
    u8 b = static_cast<u8>((color >> 8) & 0xFFu);
    u8 g = static_cast<u8>((color >> 16) & 0xFFu);
    u8 r = static_cast<u8>((color >> 24) & 0xFFu);
    return static_cast<u16>(((a >> 4) << 12) | ((b >> 4) << 8) | ((g >> 4) << 4) | (r >> 4));
}

inline u32 RGBA5551_RGBA8888(u16 color)
{
    color = static_cast<u16>((color >> 8) | (color << 8));

    u32 a = (color & 0x0001);
    u32 b = (color >> 1) & 0x1F;
    u32 g = (color >> 6) & 0x1F;
    u32 r = (color >> 11) & 0x1F;

    auto expand5 = [](u32 v) { return (v << 3) | (v >> 2); };
    auto expand1 = [](u32 v) { return v ? 0xFFu : 0x00u; };

    u32 r8 = expand5(r);
    u32 g8 = expand5(g);
    u32 b8 = expand5(b);
    u32 a8 = expand1(a);

    return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}

inline u16 RGBA5551_RGBA5551(u16 color)
{
    return swapword(color);
}

inline u32 IA88_RGBA8888(u16 color)
{
    u8 i = static_cast<u8>((color >> 8) & 0xFFu);
    u8 a = static_cast<u8>((color >> 0) & 0xFFu);
    return (static_cast<u32>(i) << 24) | (static_cast<u32>(a) << 16) | (static_cast<u32>(a) << 8) |
           (static_cast<u32>(a) << 0);
}

inline u16 IA88_RGBA4444(u16 color)
{
    u8 a4 = static_cast<u8>((color >> 4) & 0x0Fu);
    u8 i4 = static_cast<u8>((color >> 12) & 0x0Fu);
    return static_cast<u16>((a4 << 12) | (a4 << 8) | (a4 << 4) | i4);
}

inline u16 IA44_RGBA4444(u8 color)
{
    u16 i4 = static_cast<u16>((color >> 4) & 0xFu);
    u16 a4 = static_cast<u16>(color & 0xFu);
    return static_cast<u16>((i4 << 12) | (i4 << 8) | (i4 << 4) | a4);
}

inline u32 IA44_RGBA8888(u8 color)
{
    u8 i4 = static_cast<u8>((color >> 4) & 0x0Fu);
    u8 a4 = static_cast<u8>(color & 0x0Fu);
    u8 i8 = static_cast<u8>((i4 << 4) | i4);
    u8 a8 = static_cast<u8>((a4 << 4) | a4);
    return (static_cast<u32>(a8) << 24) | (static_cast<u32>(i8) << 16) | (static_cast<u32>(i8) << 8) |
           (static_cast<u32>(i8) << 0);
}

inline u16 IA31_RGBA4444(u8 color)
{
    u32 i3 = (static_cast<u32>(color) >> 1) & 0x7u;
    u32 a1 = static_cast<u32>(color) & 0x1u;
    u8 i8 = static_cast<u8>((i3 * 255 + 3) / 7);
    u8 a8 = static_cast<u8>(a1 ? 255u : 0u);
    return (static_cast<u32>(a8) << 24) | (static_cast<u32>(i8) << 16) | (static_cast<u32>(i8) << 8) |
           (static_cast<u32>(i8) << 0);
}

inline u32 IA31_RGBA8888(u8 color)
{
    u32 i3 = (static_cast<u32>(color) >> 1) & 0x7u;
    u32 a1 = static_cast<u32>(color) & 0x1u;
    u8 i8 = static_cast<u8>((i3 * 255 + 3) / 7);
    u8 a8 = static_cast<u8>(a1 ? 255u : 0u);
    return (static_cast<u32>(i8) << 24) | (static_cast<u32>(i8) << 16) | (static_cast<u32>(i8) << 8) |
           (static_cast<u32>(a8) << 0);
}

inline u16 I8_RGBA4444(u8 color)
{
    u16 i4 = static_cast<u16>(color >> 4);
    return static_cast<u16>((i4 << 12) | (i4 << 8) | (i4 << 4) | i4);
}

inline u32 I8_RGBA8888(u8 color)
{
    u8 i8 = color;
    return (static_cast<u32>(i8) << 24) | (static_cast<u32>(i8) << 16) | (static_cast<u32>(i8) << 8) |
           (static_cast<u32>(i8) << 0);
}

inline u16 I4_RGBA4444(u8 color)
{
    u16 i4 = static_cast<u16>(color & 0xFu);
    return static_cast<u16>((i4 << 12) | (i4 << 8) | (i4 << 4) | i4);
}

inline u32 I4_RGBA8888(u8 color)
{
    u8 i4 = static_cast<u8>(color & 0xFu);
    u8 i8 = static_cast<u8>((i4 << 4) | i4);
    return (static_cast<u32>(i8) << 24) | (static_cast<u32>(i8) << 16) | (static_cast<u32>(i8) << 8) |
           (static_cast<u32>(i8) << 0);
}