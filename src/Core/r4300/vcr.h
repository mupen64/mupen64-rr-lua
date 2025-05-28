/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

struct t_vcr_state {
    std::filesystem::path movie_path{};
    core_vcr_task task = task_idle;

    bool reset_pending{};

    std::optional<size_t> seek_to_frame{};
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
};

/**
 * \brief Notifies VCR engine about controller being polled
 * \param index The polled controller's index
 * \param input The controller's input data
 */
void vcr_on_controller_poll(int32_t index, core_buttons* input);

/**
 * \brief Notifies VCR engine about a new VI
 */
void vcr_on_vi();

/**
 * HACK: The VCR engine can prevent the core from pausing. Gets whether the core should be allowed to pause.
 */
bool allows_core_pause();

bool is_frame_skipped();

bool vcr_allows_core_pause();
bool vcr_allows_core_unpause();

void vcr_request_reset();
core_result vcr_read_movie_header(std::vector<uint8_t> buf, core_vcr_movie_header* header);
