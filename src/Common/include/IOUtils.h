/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

namespace IOUtils
{

// reads a file from beginning to end.
std::vector<uint8_t> read_entire_file(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return {};
    }

    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        return {};
    }

    return buffer;
}

// overwrites the contents of a file with the provided buffer.
bool write_entire_file(const std::filesystem::path &path, std::span<uint8_t> data)
{
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open())
    {
        return false;
    }

    out.write(reinterpret_cast<const char *>(data.data()), data.size());
    return out.good();
}

template <class IStreamT, class CharT = typename IStreamT::char_type, class Traits = typename IStreamT::traits_type>
    requires(std::derived_from<IStreamT, std::basic_istream<CharT, Traits>>)
inline auto iter_lines(IStreamT &stream);

namespace details
{
class IOLineSentinel
{
};

template <class IStreamT, class CharT = typename IStreamT::char_type, class Traits = typename IStreamT::traits_type>
    requires(std::derived_from<IStreamT, std::basic_istream<CharT, Traits>>)
class IOLineIterator
{
  private:
    using istream_type = IStreamT;
    using char_type = CharT;
    using traits_type = Traits;

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::basic_string_view<CharT, Traits>;
    using reference_type = std::basic_string_view<CharT, Traits>;

    template <class IStreamT2, class CharT2, class Traits2>
        requires(std::derived_from<IStreamT2, std::basic_istream<CharT2, Traits2>>)
    friend inline auto ::IOUtils::iter_lines(IStreamT2 &stream);

    value_type operator*() const { return m_line; }

    IOLineIterator &operator++()
    {
        assert(!m_stream->fail());
        std::getline(*m_stream, m_line);
        return *this;
    }

    void operator++(int)
    {
        // we can post-increment, but there is no way to return a nice value
        // so just return nothing
        ++(*this);
    }

    friend bool operator==(const IOLineIterator &iter, IOLineSentinel) { return iter.m_stream->fail(); }

    istream_type &istream() { return m_stream; }

  private:
    IOLineIterator(istream_type &stream) : m_stream(&stream), m_line()
    {
        // This class operates under the assumption that m_stream is never null.
        assert(m_stream != nullptr);
        assert(!m_stream->fail());
        std::getline(*m_stream, m_line);
    }

    istream_type *m_stream;
    std::basic_string<CharT, Traits> m_line;
};

static_assert(std::input_iterator<IOLineIterator<std::ifstream>>);
} // namespace details

// Returns an iterator over the lines of text in an input stream.
template <class IStreamT, class CharT, class Traits>
    requires(std::derived_from<IStreamT, std::basic_istream<CharT, Traits>>)
inline auto iter_lines(IStreamT &stream)
{
    return std::ranges::subrange{details::IOLineIterator<IStreamT, CharT, Traits>(stream), details::IOLineSentinel()};
}

} // namespace IOUtils