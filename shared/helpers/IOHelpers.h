#pragma once

#include <filesystem>
#include <span>
#include <vector>

/**
 * \brief Writes data to a vector at its tail end based on its size, expanding
 * it as needed \param vec The target vector \param data The source data \param
 * len The source data's size in bytes
 */
void vecwrite(std::vector<uint8_t>& vec, void* data, size_t len);

/**
 * \brief Reads a file into a buffer
 * \param path The file's path
 * \return The file's contents, or an empty vector if the operation failed
 */
std::vector<uint8_t> read_file_buffer(const std::filesystem::path& path);

/**
 * \brief Writes a buffer to a file
 * \param path The file's path
 * \param data The buffer
 * \return Whether the operation succeeded
 */
bool write_file_buffer(const std::filesystem::path& path, std::span<uint8_t> data);

/**
 * \brief Decompresses an (optionally) gzip-compressed byte vector
 * \param vec The byte vector
 * \param initial_size The initial size to allocate for the internal buffer
 * \return The decompressed byte vector
 */
std::vector<uint8_t> auto_decompress(std::vector<uint8_t>& vec, size_t initial_size = 0xB624F0);

/**
 * \brief Reads source data into the destination, advancing the source pointer
 * by <c>len</c> \param src A pointer to the source data \param dest The
 * destination buffer \param len The destination buffer's length
 */
void memread(uint8_t** src, void* dest, unsigned int len);
