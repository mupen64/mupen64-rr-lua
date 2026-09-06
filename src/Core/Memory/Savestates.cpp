/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <Core/API.hpp>
#include <Memory/FlashRAM.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Savestates.hpp>
#include <Memory/Summercart.hpp>
#include <R4300/Interrupt.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Rom.hpp>
#include <R4300/VCR.hpp>

constexpr auto rdram_device_manuf_new_fix_bit = (1U << 31);

// st that comes from no delay fix mupen, it has some differences compared to new st:
// - one frame of input is "embedded", that is the pif ram holds already fetched controller info.
// - execution continues at exception handler (after input poll) at 0x80000180.
bool g_st_old;
bool g_st_skip_dma{};

/// Represents a task to be performed by the savestate system.
struct SavestateTask
{
    /// The job to perform.
    CoreSTJob job;

    /// The savestate's source or target medium.
    CoreSTMedium medium;

    /// Callback to invoke when the task finishes. Mustn't be null.
    CoreSTCallback callback;

    /// The task's parameters. Only one field in the struct is valid at a time.
    CoreSTJobParams params{};

    /// Whether warnings, such as those about ROM compatibility, shouldn't be shown.
    bool ignore_warnings;

    bool pure{};
};

// The task vector mutex. Locked when accessing the task vector.
std::recursive_mutex g_task_mutex;

// The task vector, which contains the task queue to be performed by the savestate system.
std::vector<SavestateTask> g_tasks;

// Demarcator for new screenshot section
char screen_section[] = "SCR";

// Buffer used for storing flashram data during loading
char g_flashram_buf[1024]{};

// Buffer used for storing event queue data during loading
char g_event_queue_buf[1024]{};

// Buffer used for storing st data up to event queue
uint8_t g_first_block[0xA02BB4 - 32]{};

// The undo savestate buffer.
std::vector<uint8_t> g_undo_savestate;
void get_paths_for_task(const SavestateTask &task, std::filesystem::path &st_path, std::filesystem::path &sd_path)
{
    sd_path = g_core->get_saves_directory() / IOUtils::rom_name_to_path_component((const char *)ROM_HEADER.nom);
    sd_path.replace_extension(".vhd");
}

void load_memory_from_buffer(uint8_t *p)
{
    MiscHelpers::memread(&p, &rdram_register, sizeof(CoreRDRAMReg));
    if (rdram_register.rdram_device_manuf & rdram_device_manuf_new_fix_bit)
    {
        rdram_register.rdram_device_manuf &= ~rdram_device_manuf_new_fix_bit; // remove the trick
        g_st_skip_dma = true;                                                 // tell dma.c to skip it
    }
    MiscHelpers::memread(&p, &MI_register, sizeof(CoreMIPSReg));
    MiscHelpers::memread(&p, &pi_register, sizeof(CorePIReg));
    MiscHelpers::memread(&p, &sp_register, sizeof(CoreSPReg));
    MiscHelpers::memread(&p, &rsp_register, sizeof(CoreRSPReg));
    MiscHelpers::memread(&p, &si_register, sizeof(CoreSIReg));
    MiscHelpers::memread(&p, &vi_register, sizeof(CoreVIReg));
    MiscHelpers::memread(&p, &ri_register, sizeof(CoreRIReg));
    MiscHelpers::memread(&p, &ai_register, sizeof(CoreAIReg));
    MiscHelpers::memread(&p, &dpc_register, sizeof(CoreDPCReg));
    MiscHelpers::memread(&p, &dps_register, sizeof(CoreDPSReg));
    MiscHelpers::memread(&p, rdram, 0x800000);
    MiscHelpers::memread(&p, SP_DMEM, 0x1000);
    MiscHelpers::memread(&p, SP_IMEM, 0x1000);
    MiscHelpers::memread(&p, PIF_RAM, 0x40);

    char buf[4 * 32];
    MiscHelpers::memread(&p, buf, 24);
    load_flashram_infos(buf);

    MiscHelpers::memread(&p, tlb_LUT_r, 0x100000);
    MiscHelpers::memread(&p, tlb_LUT_w, 0x100000);

    MiscHelpers::memread(&p, &llbit, 4);
    MiscHelpers::memread(&p, reg, 32 * 8);
    for (int32_t i = 0; i < 32; i++)
    {
        MiscHelpers::memread(&p, reg_cop0 + i, 4);
        MiscHelpers::memread(&p, buf, 4); // for compatibility with old versions purpose
    }
    MiscHelpers::memread(&p, &lo, 8);
    MiscHelpers::memread(&p, &hi, 8);
    MiscHelpers::memread(&p, reg_cop1_fgr_64, 32 * 8);
    MiscHelpers::memread(&p, &FCR0, 4);
    MiscHelpers::memread(&p, &FCR31, 4);
    MiscHelpers::memread(&p, tlb_e, 32 * sizeof(tlb));
    if (!dynacore && interpcore)
        MiscHelpers::memread(&p, &interp_addr, 4);
    else
    {
        uint32_t target_addr;
        MiscHelpers::memread(&p, &target_addr, 4);
        for (char &i : invalid_code) i = 1;
        jump_to(target_addr)
    }

    MiscHelpers::memread(&p, &next_interrupt, 4);
    MiscHelpers::memread(&p, &next_vi, 4);
    MiscHelpers::memread(&p, &vi_field, 4);
}

/**
 * \brief Generates a savestate buffer.
 * \param pure If true, the generated buffer will not include the VCR freeze and video buffer. The DMA fixup will also
 * not be performed.
 * \return The generated savestate buffer.
 */
static std::vector<uint8_t> generate_savestate(bool pure)
{
    std::vector<uint8_t> b;

    b.reserve(0xB624F0);

    memset(g_flashram_buf, 0, sizeof(g_flashram_buf));
    memset(g_event_queue_buf, 0, sizeof(g_event_queue_buf));

    vcr_freeze_info freeze{};
    uint32_t movie_active = pure ? 0 : vcr_freeze(freeze);

    // NOTE: Some savestates don't have an SI interrupt in the queue, which means that a dma_si_read call which should
    // have happened prior to the save didn't happen. In that case, we "finish up" the dma by performing its final part
    // manually.
    if (!pure && get_event(SI_INT) == 0)
    {
        g_core->log_warn("[ST] Finishing up DMA...");
        for (size_t i = 0; i < 64 / 4; i++) rdram[si_register.si_dram_addr / 4 + i] = std::byteswap(PIF_RAM[i]);
        update_count();
        add_interrupt_event(SI_INT, 0x900);
        g_st_skip_dma = true;
    }

    // NOTE: This saving needs to be done **after** the fixing block, as it is now. See previous regression in
    // f9d58f639c798cbc26bbb808b1c3dbd834ffe2d9.
    save_flashram_infos(g_flashram_buf);
    const int32_t event_queue_len = save_eventqueue_infos(g_event_queue_buf);

    MiscHelpers::vecwrite(b, rom_md5, 32);
    MiscHelpers::vecwrite(b, &rdram_register, sizeof(CoreRDRAMReg));
    MiscHelpers::vecwrite(b, &MI_register, sizeof(CoreMIPSReg));
    MiscHelpers::vecwrite(b, &pi_register, sizeof(CorePIReg));
    MiscHelpers::vecwrite(b, &sp_register, sizeof(CoreSPReg));
    MiscHelpers::vecwrite(b, &rsp_register, sizeof(CoreRSPReg));
    MiscHelpers::vecwrite(b, &si_register, sizeof(CoreSIReg));
    MiscHelpers::vecwrite(b, &vi_register, sizeof(CoreVIReg));
    MiscHelpers::vecwrite(b, &ri_register, sizeof(CoreRIReg));
    MiscHelpers::vecwrite(b, &ai_register, sizeof(CoreAIReg));
    MiscHelpers::vecwrite(b, &dpc_register, sizeof(CoreDPCReg));
    MiscHelpers::vecwrite(b, &dps_register, sizeof(CoreDPSReg));
    MiscHelpers::vecwrite(b, rdram, 0x800000);
    MiscHelpers::vecwrite(b, SP_DMEM, 0x1000);
    MiscHelpers::vecwrite(b, SP_IMEM, 0x1000);
    MiscHelpers::vecwrite(b, PIF_RAM, 0x40);
    MiscHelpers::vecwrite(b, g_flashram_buf, 24);
    MiscHelpers::vecwrite(b, tlb_LUT_r, 0x100000);
    MiscHelpers::vecwrite(b, tlb_LUT_w, 0x100000);
    MiscHelpers::vecwrite(b, &llbit, 4);
    MiscHelpers::vecwrite(b, reg, 32 * 8);
    for (size_t i = 0; i < 32; i++)
        MiscHelpers::vecwrite(b, reg_cop0 + i, 8); // *8 for compatibility with old versions purpose
    MiscHelpers::vecwrite(b, &lo, 8);
    MiscHelpers::vecwrite(b, &hi, 8);
    MiscHelpers::vecwrite(b, reg_cop1_fgr_64, 32 * 8);
    MiscHelpers::vecwrite(b, &FCR0, 4);
    MiscHelpers::vecwrite(b, &FCR31, 4);
    MiscHelpers::vecwrite(b, tlb_e, 32 * sizeof(tlb));
    if (!dynacore && interpcore)
        MiscHelpers::vecwrite(b, &interp_addr, 4);
    else
        MiscHelpers::vecwrite(b, &PC->addr, 4);
    MiscHelpers::vecwrite(b, &next_interrupt, 4);
    MiscHelpers::vecwrite(b, &next_vi, 4);
    MiscHelpers::vecwrite(b, &vi_field, 4);
    MiscHelpers::vecwrite(b, g_event_queue_buf, event_queue_len);

    if (!pure)
    {
        MiscHelpers::vecwrite(b, &movie_active, sizeof(movie_active));
        if (movie_active)
        {
            MiscHelpers::vecwrite(b, &freeze.size, sizeof(freeze.size));
            MiscHelpers::vecwrite(b, &freeze.uid, sizeof(freeze.uid));
            MiscHelpers::vecwrite(b, &freeze.current_sample, sizeof(freeze.current_sample));
            MiscHelpers::vecwrite(b, &freeze.current_vi, sizeof(freeze.current_vi));
            MiscHelpers::vecwrite(b, &freeze.length_samples, sizeof(freeze.length_samples));
            MiscHelpers::vecwrite(b, freeze.input_buffer.data(), freeze.input_buffer.size() * sizeof(CoreButtons));
        }

        if (g_core->mge_available() && g_core->cfg->st_screenshot)
        {
            int32_t width;
            int32_t height;
            g_core->video_get_video_size(&width, &height);
            g_core->log_trace(std::format("Writing screen buffer to savestate, width: {}, height: {}", width, height));

            void *video = malloc(width * height * 4);
            g_core->copy_video(video);

            MiscHelpers::vecwrite(b, screen_section, sizeof(screen_section));
            MiscHelpers::vecwrite(b, &width, sizeof(width));
            MiscHelpers::vecwrite(b, &height, sizeof(height));
            MiscHelpers::vecwrite(b, video, width * height * 4);

            free(video);
        }
    }

    return b;
}

void savestates_save_immediate_impl(const SavestateTask &task)
{
    // TODO: Reimplement timing

    const auto st = generate_savestate(task.pure);

    if (task.medium == CoreSTMedium::Path)
    {
        // Always save summercart for some reason
        std::filesystem::path new_st_path = task.params.path;
        std::filesystem::path new_sd_path = "";
        get_paths_for_task(task, new_st_path, new_sd_path);
        if (g_core->cfg->use_summercart) save_summercart(new_sd_path);

        const auto compressor = g_core->cfg->st_lz4 ? MiscHelpers::Compressor::Lz4 : MiscHelpers::Compressor::Gzip;
        const auto compressed = MiscHelpers::compress(compressor, st);

        if (compressed.empty())
        {
            task.callback(CoreSTCallbackInfo{.result = CoreResult::ST_FileWriteError,
                              .job = task.job,
                              .medium = task.medium,
                              .params = task.params},
                st);
            return;
        }

        // write compressed st to disk
        if (!IOUtils::write_entire_file(new_st_path, compressed))
        {
            task.callback(CoreSTCallbackInfo{.result = CoreResult::ST_FileWriteError,
                              .job = task.job,
                              .medium = task.medium,
                              .params = task.params},
                st);
            return;
        }
    }

    task.callback(
        CoreSTCallbackInfo{.result = CoreResult::Res_Ok, .job = task.job, .medium = task.medium, .params = task.params},
        st);
    g_core->callbacks.save_state();
}

void savestates_load_immediate_impl(const SavestateTask &task)
{
    // This might have been set previously by a save operation. Keeping it breaks loading because we might skip DMA when
    // it's not needed.
    g_st_skip_dma = false;

    // TODO: Reimplement timing

    memset(g_event_queue_buf, 0, sizeof(g_event_queue_buf));

    std::filesystem::path new_st_path = task.params.path;
    std::filesystem::path new_sd_path = "";
    get_paths_for_task(task, new_st_path, new_sd_path);

    if (g_core->cfg->use_summercart) load_summercart(new_sd_path);

    std::vector<uint8_t> st_buf;

    switch (task.medium)
    {
    case CoreSTMedium::Path:
        st_buf = IOUtils::read_entire_file(new_st_path);
        break;
    case CoreSTMedium::Memory:
        st_buf = task.params.buffer;
        break;
    default:
        assert(false);
    }

    if (st_buf.empty())
    {
        task.callback(
            CoreSTCallbackInfo{
                .result = CoreResult::ST_NotFound, .job = task.job, .medium = task.medium, .params = task.params},
            {});
        return;
    }

    std::vector<uint8_t> decompressed_buf = MiscHelpers::auto_decompress(st_buf, 0xB624F0);

    if (decompressed_buf.empty())
    {
        task.callback(CoreSTCallbackInfo{.result = CoreResult::ST_DecompressionError,
                          .job = task.job,
                          .medium = task.medium,
                          .params = task.params},
            {});
        return;
    }

    // BUG (PRONE): we arent allowed to hold on to a vector element pointer
    // find another way of doing this
    auto ptr = decompressed_buf.data();

    // compare current rom hash with one stored in state
    char md5[33] = {0};
    MiscHelpers::memread(&ptr, &md5, 32);

    if (!task.ignore_warnings && memcmp(md5, rom_md5, 32))
    {
        auto result = g_core->show_ask_dialog(CORE_DLG_ST_HASH_MISMATCH,
            std::format("The savestate was created on a rom with hash {}, but is being loaded on another rom.\r\nThe "
                        "emulator may crash. Are you sure you want to continue?",
                md5)
                .c_str(),
            "Savestate", true);

        if (!result)
        {
            task.callback(
                CoreSTCallbackInfo{
                    .result = CoreResult::Res_Cancelled, .job = task.job, .medium = task.medium, .params = task.params},
                {});
            return;
        }
    }

    // new version does one bigass gzread for first part of .st (static size)
    MiscHelpers::memread(&ptr, g_first_block, sizeof(g_first_block));

    CoreSIReg si_reg;
    std::memcpy(&si_reg, &g_first_block[0xDC - 0x20], sizeof(si_reg));
    const bool si_register_valid = check_register_validity(&si_reg);
    const bool flashram_infos_valid = check_flashram_infos(&g_first_block[0x8021F0 - 0x20]);
    assert(si_register_valid && flashram_infos_valid && "Savestate contains invalid DMA register contents");
    if (!si_register_valid || !flashram_infos_valid)
    {
        task.callback(CoreSTCallbackInfo{.result = CoreResult::ST_InvalidRegisters,
                          .job = task.job,
                          .medium = task.medium,
                          .params = task.params},
            {});
        return;
    }

    // now read interrupt queue into buf
    int32_t len;
    for (len = 0; len < sizeof(g_event_queue_buf); len += 8)
    {
        uint32_t event_type;
        MiscHelpers::memread(&ptr, &event_type, sizeof(event_type));
        std::memcpy(g_event_queue_buf + len, &event_type, sizeof(event_type));
        if (event_type == 0xFFFFFFFF) break;
        MiscHelpers::memread(&ptr, g_event_queue_buf + len + 4, 4);
    }
    if (len == sizeof(g_event_queue_buf))
    {
        // Exhausted the buffer and still no terminator. Prevents the buffer overflow "Queuecrush".
        task.callback(CoreSTCallbackInfo{.result = CoreResult::ST_EventQueueTooLong,
                          .job = task.job,
                          .medium = task.medium,
                          .params = task.params},
            {});
        return;
    }

    uint32_t is_movie;
    MiscHelpers::memread(&ptr, &is_movie, sizeof(is_movie));

    if (is_movie)
    {
        // this .st is part of a movie, we need to overwrite our current movie buffer
        // hash matches, load and verify rest of the data
        vcr_freeze_info freeze{};

        MiscHelpers::memread(&ptr, &freeze.size, sizeof(freeze.size));
        MiscHelpers::memread(&ptr, &freeze.uid, sizeof(freeze.uid));
        MiscHelpers::memread(&ptr, &freeze.current_sample, sizeof(freeze.current_sample));
        MiscHelpers::memread(&ptr, &freeze.current_vi, sizeof(freeze.current_vi));
        MiscHelpers::memread(&ptr, &freeze.length_samples, sizeof(freeze.length_samples));

        freeze.input_buffer.resize(sizeof(CoreButtons) * (freeze.length_samples + 1));
        MiscHelpers::memread(&ptr, freeze.input_buffer.data(), freeze.input_buffer.size());

        const auto code = vcr_unfreeze(freeze);

        if (!task.ignore_warnings && code != CoreResult::Res_Ok && vcr_get_task() != CoreVCRTask::Idle)
        {
            std::string err_str = "Failed to restore movie, ";
            switch (code)
            {
            case CoreResult::VCR_NotFromThisMovie:
                err_str += "the savestate is not from this movie.";
                break;
            case CoreResult::VCR_InvalidFrame:
                err_str += "the savestate frame is outside the bounds of the movie.";
                break;
            case CoreResult::VCR_InvalidFormat:
                err_str += "the savestate freeze buffer format is invalid.";
                break;
            default:
                err_str += "an unknown error has occured.";
                break;
            }
            err_str += " Loading the savestate might desynchronize the movie.\r\nAre you sure you want to continue?";

            const auto result =
                g_core->show_ask_dialog(CORE_DLG_ST_UNFREEZE_WARNING, err_str.c_str(), "Savestate", true);
            if (!result)
            {
                task.callback(CoreSTCallbackInfo{.result = CoreResult::Res_Cancelled,
                                  .job = task.job,
                                  .medium = task.medium,
                                  .params = task.params},
                    {});
                goto failedLoad;
            }
        }
    }
    else
    {
        if (!task.ignore_warnings &&
            (vcr_get_task() == CoreVCRTask::Recording || vcr_get_task() == CoreVCRTask::Playback))
        {
            const auto result = g_core->show_ask_dialog(CORE_DLG_ST_NOT_FROM_MOVIE,
                "The savestate is not from a movie. Loading it might desynchronize the "
                "movie.\r\nAre you sure you want to continue?",
                "Savestate", true);
            if (!result)
            {
                task.callback(CoreSTCallbackInfo{.result = CoreResult::Res_Cancelled,
                                  .job = task.job,
                                  .medium = task.medium,
                                  .params = task.params},
                    {});
                return;
            }
        }

        // at this point we know the savestate is safe to be loaded (done after else block)
    }

    {
        g_core->log_trace(
            std::format("[Savestates] {} bytes remaining", decompressed_buf.size() - (ptr - decompressed_buf.data())));
        int32_t video_width = 0;
        int32_t video_height = 0;
        void *video_buffer = nullptr;
        if (decompressed_buf.size() - (ptr - decompressed_buf.data()) > 0)
        {
            char scr_section[sizeof(screen_section)] = {0};
            MiscHelpers::memread(&ptr, scr_section, sizeof(screen_section));

            if (!memcmp(scr_section, screen_section, sizeof(screen_section)))
            {
                g_core->log_trace(std::format("[Savestates] Restoring screen buffer..."));
                MiscHelpers::memread(&ptr, &video_width, sizeof(video_width));
                MiscHelpers::memread(&ptr, &video_height, sizeof(video_height));

                const auto remaining_space = decompressed_buf.size() - (ptr - decompressed_buf.data());
                const auto has_enough_space = remaining_space >= video_width * video_height * 4;
                if (has_enough_space)
                {
                    video_buffer = malloc(video_width * video_height * 4);
                    MiscHelpers::memread(&ptr, video_buffer, video_width * video_height * 4);
                }
                else
                    g_core->log_error(std::format("[Savestates] Not enough space left in buffer for screen buffer"));
            }
        }

        // so far loading success! overwrite memory
        load_eventqueue_infos(g_event_queue_buf);
        load_memory_from_buffer(g_first_block);

        // NOTE: We don't want to restore screen buffer while seeking, since it creates a int16_t ugly flicker when the
        // movie restarts by loading state
        if (g_core->mge_available() && video_buffer && !vcr_is_seeking())
        {
            int32_t current_width, current_height;
            g_core->video_get_video_size(&current_width, &current_height);
            if (current_width == video_width && current_height == video_height)
            {
                g_core->load_screen(video_buffer);
                free(video_buffer);
            }
        }
    }

    g_core->callbacks.load_state();
    task.callback(
        CoreSTCallbackInfo{.result = CoreResult::Res_Ok, .job = task.job, .medium = task.medium, .params = task.params},
        decompressed_buf);

failedLoad:
    // legacy .st fix, makes BEQ instruction ignore jump, because .st writes new address explictly.
    // This should cause issues anyway but libultra seems to be flexible (this means there's a chance it fails).
    // For safety, load .sts in dynarec because it completely avoids this issue by being differently coded
    g_st_old = (interp_addr == 0x80000180 || PC->addr == 0x80000180);
    // doubled because can't just reuse this variable
    if (interp_addr == 0x80000180 || (PC->addr == 0x80000180 && !dynacore)) g_vr_beq_ignore_jmp = true;
    if (!dynacore && interpcore)
    {
        // g_core->log_info(".st jump: {:#06x}, stopped here:{:#06x}", interp_addr, last_addr);
        last_addr = interp_addr;
    }
    else
    {
        // g_core->log_info(".st jump: {:#06x}, stopped here:{:#06x}", PC->addr, last_addr);
        last_addr = PC->addr;
    }
}

/**
 * Simplifies the task queue by removing duplicates. Only slot-based tasks are affected for now.
 */
void savestates_simplify_tasks()
{
    std::scoped_lock lock(g_task_mutex);
    g_core->log_info("[ST] Simplifying task queue...");

    std::vector<size_t> duplicate_indicies{};

    // De-dup slot-based save tasks
    // 1. Loop through all tasks
    for (size_t i = 0; i < g_tasks.size(); i++)
    {
        const auto &task = g_tasks[i];

        if (task.medium != CoreSTMedium::Path) continue;

        // 2. If a path task is detected, loop through all other tasks up to the next load task to find duplicates
        for (size_t j = i + 1; j < g_tasks.size(); j++)
        {
            const auto &other_task = g_tasks[j];

            if (other_task.job == CoreSTJob::Load)
            {
                break;
            }

            if (other_task.medium == CoreSTMedium::Path && task.params.path == other_task.params.path)
            {
                g_core->log_trace(std::format("[ST] Found duplicate slot task at index {}", j));
                duplicate_indicies.push_back(j);
            }
        }
    }

    g_tasks = MiscHelpers::erase_indices(g_tasks, duplicate_indicies);
}

/**
 * Warns if a savestate load task is scheduled after a save task.
 */
void savestates_warn_if_load_after_save()
{
    std::scoped_lock lock(g_task_mutex);

    bool encountered_load = false;
    for (const auto &task : g_tasks)
    {
        if (task.job == CoreSTJob::Save && encountered_load)
        {
            // tood
            g_core->log_warn("[ST] A savestate save task is scheduled after a load task. This may cause "
                             "unexpected behavior for the caller.");
            break;
        }

        if (task.job == CoreSTJob::Load)
        {
            encountered_load = true;
        }
    }
}

/**
 * Logs the current task queue.
 */
void savestates_log_tasks()
{
    std::scoped_lock lock(g_task_mutex);
    g_core->log_info("[ST] Begin task dump");
    savestates_warn_if_load_after_save();
    for (const auto &task : g_tasks)
    {
        std::string job_str = (task.job == CoreSTJob::Save) ? "Save" : "Load";
        std::string medium_str;
        switch (task.medium)
        {
        case CoreSTMedium::Path:
            medium_str = "Path";
            break;
        case CoreSTMedium::Memory:
            medium_str = "Memory";
            break;
        default:
            medium_str = "Unknown";
            break;
        }
        g_core->log_info(std::format("[ST] \tTask: Job = {}, Medium = {}", job_str, medium_str));
    }
    g_core->log_info("[ST] End task dump");
}

/**
 * Inserts a save operation at the start of the queue (whose callback assigns the undo savestate buffer) if the task
 * queue contains one or more load operations.
 */
void savestates_create_undo_point()
{
    if (!g_core->cfg->st_undo_load)
    {
        return;
    }

    bool queue_contains_load =
        std::ranges::any_of(g_tasks, [](const SavestateTask &task) { return task.job == CoreSTJob::Load; });

    if (!queue_contains_load)
    {
        g_core->log_trace("[ST] Skipping undo point creation: no load in queue.");
        return;
    }

    g_core->log_trace("[ST] Inserting undo point creation into task queue...");

    const SavestateTask task = {
        .job = CoreSTJob::Save,
        .medium = CoreSTMedium::Memory,
        .callback =
            [](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buffer) {
                if (info.result != CoreResult::Res_Ok)
                {
                    return;
                }

                std::scoped_lock lock(g_task_mutex);
                g_undo_savestate = buffer;
            },
        .params =
            {
                .buffer = {},
            },
        .ignore_warnings = true,
    };

    g_tasks.insert(g_tasks.begin(), task);
}

void st_do_work()
{
    std::scoped_lock lock(g_task_mutex);

    if (g_tasks.empty())
    {
        return;
    }

    savestates_simplify_tasks();
    savestates_create_undo_point();
    savestates_simplify_tasks();
    savestates_log_tasks();

    for (const auto &task : g_tasks)
    {
        g_core->log_info(std::format("---------- Savestate {}:", (task.job == CoreSTJob::Save) ? "save" : "load"));

        if (task.job == CoreSTJob::Save)
        {
            savestates_save_immediate_impl(task);
        }
        else
        {
            savestates_load_immediate_impl(task);
        }

        g_core->log_warn("[ST] INTERRUPT QUEUE AT END OF ST TASK:");
        print_queue();
    }
    g_tasks.clear();
}

void st_on_core_stop()
{
    std::scoped_lock lock(g_task_mutex);
    g_tasks.clear();
    g_undo_savestate.clear();
}

/**
 * Gets whether work can currently be enqueued.
 */
bool can_push_work()
{
    return core_executing;
}

bool st_do_file(
    const std::filesystem::path &path, const CoreSTJob job, const CoreSTCallback &callback, bool ignore_warnings)
{
    std::scoped_lock lock(g_task_mutex);

    if (!can_push_work())
    {
        g_core->log_trace("[ST] do_file: Can't enqueue work.");
        if (callback)
        {
            callback(CoreSTCallbackInfo{.result = CoreResult::ST_CoreNotLaunched,
                         .job = job,
                         .medium = CoreSTMedium::Path,
                         .params = {.path = path}},
                {});
        }
        return false;
    }

    auto internal_callback_wrapper = [=](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buffer) {
        g_core->st_pre_callback(info, buffer);
        if (callback)
        {
            callback(info, buffer);
        }
    };

    const SavestateTask task = {
        .job = job,
        .medium = CoreSTMedium::Path,
        .callback = internal_callback_wrapper,
        .params = {.path = path},
        .ignore_warnings = ignore_warnings,
    };

    g_tasks.insert(g_tasks.begin(), task);
    return true;
}

bool st_do_memory(
    const std::vector<uint8_t> &buffer, const CoreSTJob job, const CoreSTCallback &callback, bool ignore_warnings)
{
    std::scoped_lock lock(g_task_mutex);

    if (!can_push_work())
    {
        g_core->log_trace("[ST] do_memory: Can't enqueue work.");
        if (callback)
        {
            callback(CoreSTCallbackInfo{.result = CoreResult::ST_CoreNotLaunched,
                         .job = job,
                         .medium = CoreSTMedium::Memory,
                         .params = {.buffer = buffer}},
                {});
        }
        return false;
    }

    auto internal_callback_wrapper = [=](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buffer) {
        g_core->st_pre_callback(info, buffer);
        if (callback)
        {
            callback(info, buffer);
        }
    };

    const SavestateTask task = {
        .job = job,
        .medium = CoreSTMedium::Memory,
        .callback = internal_callback_wrapper,
        .params = {.buffer = buffer},
        .ignore_warnings = ignore_warnings,
    };

    g_tasks.insert(g_tasks.begin(), task);
    return true;
}

void st_get_undo_savestate(std::vector<uint8_t> &buffer)
{
    std::scoped_lock lock(g_task_mutex);
    buffer.clear();
    buffer = g_undo_savestate;
}

bool st_sync_hash(const std::function<void(uint64_t hash)> &callback)
{
    std::scoped_lock lock(g_task_mutex);
    if (!can_push_work()) return false;

    auto internal_callback_wrapper = [=](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buffer) {
        if (!callback) return;
        const auto hash = FNV1A::hash(buffer);
        callback(hash);
    };

    const SavestateTask task = {.job = CoreSTJob::Save,
        .medium = CoreSTMedium::Memory,
        .callback = internal_callback_wrapper,
        .params = {},
        .ignore_warnings = true,
        .pure = true};

    g_tasks.insert(g_tasks.begin(), task);
    return true;
}
