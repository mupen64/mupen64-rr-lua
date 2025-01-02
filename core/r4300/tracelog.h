#pragma once

#include <filesystem>

namespace tracelog
{
    /**
     * \return Whether the trace logger is active
     */
    bool active();

    /**
     * \brief Starts trace logging to the specified file
     * \param path The output path
     * \param binary Whether log output is in a binary format
     * \param append Whether log output will be appended to the file
     */
    void start(std::filesystem::path path, bool binary, bool append = false);

    /**
     * \brief Stops trace logging
     */
    void stop();

    /**
     * \brief Logs a dynarec-generated instruction
     */
    void log_interp_ops();

    /**
     * \brief Logs a pure interp instruction
     */
    void log_pure();
} // namespace tracelog
