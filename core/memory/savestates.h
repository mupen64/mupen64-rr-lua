/**
 * Mupen64 - savestates.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 **/
#pragma once

#include <filesystem>
#include <functional>
#include <shared/types/CoreTypes.h>

extern bool g_st_skip_dma;
extern bool g_st_old;

namespace Savestates
{
    enum class Job {
        // A save operation
        Save,
        // A load operation
        Load
    };

    enum class Medium {
        // The target medium is a slot (0-9).
        Slot,
        // The target medium is a file with a path.
        Path,
        // The target medium is in-memory.
        Memory
    };

    using t_savestate_callback = std::function<void(CoreResult result, const std::vector<uint8_t>&)>;

    /**
     * \brief Initializes the savestate system
     */
    void init();

    /**
     * \brief Does the pending savestate work.
     * \warning This function must only be called from the emulation thread. Other
     * callers must use the <c>savestates_do_x</c> family.
     */
    void do_work();

    /**
     * Clears the work queue.
     */
    void clear_work_queue();

    /**
     * \brief Executes a savestate operation to a path
     * \param path The savestate's path
     * \param job The job to set
     * \param callback The callback to call when the operation is complete.
     * \param ignore_warnings Whether warnings, such as those about ROM
     * compatibility, shouldn't be shown. \warning The operation won't complete
     * immediately. Must be called via AsyncExecutor unless calls are originating
     * from the emu thread.
     */
    void do_file(const std::filesystem::path& path, Job job, const t_savestate_callback& callback = nullptr, bool ignore_warnings = false);

    /**
     * \brief Executes a savestate operation to a slot
     * \param slot The slot to construct the savestate path with.
     * \param job The job to set
     * \param callback The callback to call when the operation is complete.
     * \param ignore_warnings Whether warnings, such as those about ROM
     * compatibility, shouldn't be shown. \warning The operation won't complete
     * immediately. Must be called via AsyncExecutor unless calls are originating
     * from the emu thread.
     */
    void do_slot(int32_t slot, Job job, const t_savestate_callback& callback = nullptr, bool ignore_warnings = false);

    /**
     * Executes a savestate operation in-memory.
     * \param buffer The buffer to use for the operation. Can be empty if the <see
     * cref="job"/> is <see cref="e_st_job::save"/>. \param job The job to set.
     * \param callback The callback to call when the operation is complete.
     * \param ignore_warnings Whether warnings, such as those about ROM
     * compatibility, shouldn't be shown. \warning The operation won't complete
     * immediately. Must be called via AsyncExecutor unless calls are originating
     * from the emu thread.
     */
    void do_memory(const std::vector<uint8_t>& buffer, Job job, const t_savestate_callback& callback = nullptr, bool ignore_warnings = false);

    /**
     * Gets the undo savestate buffer. Will be empty will no undo savestate is
     * available.
     */
    std::vector<uint8_t> get_undo_savestate();

} // namespace Savestates
