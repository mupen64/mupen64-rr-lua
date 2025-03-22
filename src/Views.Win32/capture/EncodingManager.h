/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


/**
 * Provides encoding functionality to the view.
 * \warning This namespace is not thread-safe unless otherwise specified.
 */
namespace EncodingManager
{
    /**
     * \brief Synchronization modes the encoding manager can abide by
     */
    enum class Sync : int {
        /**
         * \brief Video and Audio streams are not kept in sync
         */
        None,

        /**
         * \brief The audio stream dictates the video stream's rate
         */
        Audio,

        /**
         * \brief The video stream dictates the audio stream's rate
         */
        Video,
    };

    /**
     * \brief Initializes the encoding manager
     */
    void init();

    /**
     * \brief Whether a capture is currently running
     * \remarks This method is thread-safe.
     */
    bool is_capturing();

    /**
     * \brief Starts capturing a video.
     * \param path The movie's path
     * \param encoder_type The encoder to use for capturing
     * \param ask_for_encoding_settings Whether the codec dialog should be shown. If false, the previously used codec or the default one will be used.
     * \param callback The callback that will be invoked when the operation completes. Can be null.
     * \remarks This function must be called from a thread that isn't directly or indirectly interlocked with the emulator thread. Emulation will be paused until the operation completes.
     */
    void start_capture(std::filesystem::path path, cfg_encoder_type encoder_type, bool ask_for_encoding_settings = true, const std::function<void(bool)>& callback = nullptr);

    /**
     * \brief Stops capturing a video.
     * \param callback The callback that will be invoked when the operation completes. Can be null.
     * \remarks This function must be called from a thread that isn't directly or indirectly interlocked with the emulator thread. Emulation will be paused until the operation completes.
     */
    void stop_capture(const std::function<void(bool)>& callback = nullptr);

    /**
     * \brief Notifies the encoding manager of a new VI
     */
    void at_vi();

    /**
     * \brief Notifies the encoding manager of the audio changing
     */
    void ai_len_changed();

    /**
     * Gets the current video frame index.
     */
    size_t get_video_frame();

    /**
     * Gets the current output path.
     */
    std::filesystem::path get_current_path();
} // namespace EncodingManager
