#include "IOHelpers.h"
#include <libdeflate.h>
#include <shared/services/LoggingService.h>

void vecwrite(std::vector<uint8_t>& vec, void* data, size_t len)
{
    vec.resize(vec.size() + len);
    memcpy(vec.data() + (vec.size() - len), data, len);
}

std::vector<uint8_t> read_file_buffer(const std::filesystem::path& path)
{
    FILE* f = fopen(path.string().c_str(), "rb");

    if (!f)
    {
        return {};
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> b;
    b.resize(len);

    fread(b.data(), sizeof(uint8_t), len, f);

    fclose(f);
    return b;
}

bool write_file_buffer(const std::filesystem::path& path, std::span<uint8_t> data)
{
    FILE* f = fopen(path.string().c_str(), "wb");

    if (!f)
    {
        return false;
    }

    fwrite(data.data(), sizeof(uint8_t), data.size(), f);
    fclose(f);

    return true;
}

std::vector<uint8_t> auto_decompress(std::vector<uint8_t>& vec, size_t initial_size)
{
    if (vec.size() < 2 || vec[0] != 0x1F && vec[1] != 0x8B)
    {
        // vec is decompressed already
        g_shared_logger->info("Already decompressed");
        // we need a copy, not ref
        std::vector<uint8_t> out_vec = vec;
        return out_vec;
    }

    // we dont know what the decompressed size is, so we reallocate a buffer until
    // we find the right size lol
    size_t buf_size = initial_size;
    auto out_buf = static_cast<uint8_t*>(malloc(buf_size));
    auto decompressor = libdeflate_alloc_decompressor();
    while (true)
    {
        out_buf = static_cast<uint8_t*>(realloc(out_buf, buf_size));
        size_t actual_size = 0;
        auto result = libdeflate_gzip_decompress(decompressor, vec.data(), vec.size(), out_buf, buf_size, &actual_size);
        if (result == LIBDEFLATE_SHORT_OUTPUT || result == LIBDEFLATE_INSUFFICIENT_SPACE)
        {
            g_shared_logger->info("Buffer size of {} insufficient...", buf_size);
            buf_size *= 2;
            continue;
        }
        buf_size = actual_size;
        break;
    }
    libdeflate_free_decompressor(decompressor);

    g_shared_logger->info("Found size {}...", buf_size);
    out_buf = static_cast<uint8_t*>(realloc(out_buf, buf_size));
    std::vector<uint8_t> out_vec;
    out_vec.resize(buf_size);
    memcpy(out_vec.data(), out_buf, buf_size);
    free(out_buf);
    return out_vec;
}

void memread(uint8_t** src, void* dest, unsigned int len)
{
    memcpy(dest, *src, len);
    *src += len;
}
