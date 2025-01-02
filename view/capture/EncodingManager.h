#pragma once
#include <filesystem>
#include <shared/Config.hpp>

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
     * \brief Whether a capture is currently running
     * \remarks This method is thread-safe.
     */
    bool is_capturing();

    /**
     * \brief Initializes the encoding manager
     */
    void init();

    /**
     * \brief Starts capturing a video
     * \param path The movie's path
     * \param encoder_type The encoder to use for capturing
     * \param ask_for_encoding_settings Whether the codec dialog should be shown. If
     * false, the previously used codec or the default one will be used. \return
     * Whether the operation was successful
     */
    bool start_capture(std::filesystem::path path, EncoderType encoder_type, bool ask_for_encoding_settings = true);

    /**
     * \brief Stops capturing a video
     * \return Whether the operation was successful
     */
    bool stop_capture();

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
