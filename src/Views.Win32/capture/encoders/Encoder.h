/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

class Encoder {
public:
    struct Params {
        /**
         * \brief The video file's path
         */
        std::filesystem::path path;
        /**
         * \brief The video stream's width
         */
        uint32_t width;
        /**
         * \brief The video stream's height
         */
        uint32_t height;
        /**
         * \brief The video stream's FPS
         */
        uint32_t fps;
        /**
         * \brief The audio stream's bitrate
         */
        uint32_t arate;
        /**
         * \brief Ask the user for encoding settings
         */
        bool ask_for_encoding_settings;
    };

    /**
     * \brief Destroys the encoder and cleans up its resources
     */
    virtual ~Encoder() = default;

    /**
     * \brief Starts encoding
     * \param params The parameters to encode with
     * \return The error message, or an empty string if the operation succeeded.
     */
    virtual std::wstring start(Params params) = 0;

    /**
     * \brief Stops encoding
     * \return Whether the operation succeeded
     */
    virtual bool stop() = 0;

    /**
     * \brief Adds one frame of video data
     * \param image The video buffer. Must be freed with the provided video free function.
     * \return Whether the operation succeeded
     */
    virtual bool append_video(uint8_t* image) = 0;

    /**
     * \brief Adds samples of audio data
     * \param audio The audio buffer. Must be freed with the provided audio free function.
     * \param length The audio length
     * \param bitrate The audio bitrate
     * \return Whether the operation succeeded
     */
    virtual bool append_audio(uint8_t* audio, size_t length, uint8_t bitrate) = 0;
};
