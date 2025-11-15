/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <core_api.h>

struct t_vcr_state
{
    std::filesystem::path movie_path{};
    core_vcr_task task = task_idle;

    bool reset_pending{};

    std::optional<size_t> seek_to_frame{};
    size_t seek_start_sample{};
    bool seek_pause_at_end{};
    bool seek_savestate_loading{};
    std::unordered_map<size_t, std::vector<uint8_t>> seek_savestates{};

    bool warp_modify_active{};
    size_t warp_modify_first_difference_frame{};

    core_vcr_movie_header hdr{};
    std::vector<core_buttons> inputs{};

    int32_t current_sample = -1;
    int32_t current_vi = -1;

    bool reset_requested{};
    std::queue<std::function<void()>> post_controller_poll_callbacks{};
};

/**
 * \brief The movie freeze buffer, which is used to store the movie (with only essential data) associated with a
 * savestate inside the savestate.
 */
struct vcr_freeze_info
{
    uint32_t size{};
    uint32_t uid{};
    uint32_t current_sample{};
    uint32_t current_vi{};
    uint32_t length_samples{};
    std::vector<core_buttons> input_buffer{};
};

extern t_vcr_state vcr;
extern std::mutex vcr_mtx;

/**
 * \brief Notifies VCR engine about controller being polled
 * \param index The polled controller's index
 * \param input The controller's input data
 */
void vcr_on_controller_poll(int32_t index, core_buttons *input);

/**
 * \brief Notifies VCR engine about a new VI
 */
void vcr_on_vi();

/**
 * HACK: The VCR engine can prevent the core from pausing. Gets whether the core should be allowed to pause.
 */
bool vcr_allows_core_pause();
bool vcr_allows_core_unpause();
bool vcr_is_frame_skipped();
void vcr_request_reset();
core_result vcr_read_movie_header(std::vector<uint8_t> buf, core_vcr_movie_header *header);
core_result vcr_parse_header(std::filesystem::path path, core_vcr_movie_header *header);
core_result vcr_read_movie_inputs(std::filesystem::path path, std::vector<core_buttons> &inputs);
core_result vcr_start_playback(std::filesystem::path path);
core_result vcr_start_record(std::filesystem::path path, uint16_t flags, std::string author, std::string description);
core_result vcr_continue_recording();
core_result vcr_replace_author_info(const std::filesystem::path &path, std::string_view author,
                                    std::string_view description);
core_vcr_seek_info vcr_get_seek_info();
core_result vcr_begin_seek(std::string str, bool pause_at_end);
void vcr_stop_seek();
bool vcr_is_seeking();
bool vcr_freeze(vcr_freeze_info &freeze);
core_result vcr_unfreeze(const vcr_freeze_info &freeze);
core_result vcr_write_backup();
core_result vcr_stop_all();
std::filesystem::path vcr_get_path();
core_vcr_task vcr_get_task();
uint32_t vcr_get_length_samples();
uint32_t vcr_get_length_vis();
int32_t vcr_get_current_vi();
std::vector<core_buttons> vcr_get_inputs();
core_result vcr_begin_warp_modify(const std::vector<core_buttons> &inputs);
bool vcr_get_warp_modify_status();
size_t vcr_get_warp_modify_first_difference_frame();
void vcr_get_seek_savestate_frames(std::unordered_map<size_t, bool> &map);
bool vcr_has_seek_savestate_at_frame(size_t frame);
