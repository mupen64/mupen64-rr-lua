/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "core_types.h"
#include <CommonPCH.h>
// #include <PlatformService.h>
#include <Core.h>
#include <cassert>
#include <cheats.h>
#include <filesystem>
#include <format>
#include <include/core_api.h>
#include <iterator>
#include <r4300/r4300.h>
#include <r4300/rom.h>
#include <r4300/vcr.h>

using namespace std::string_view_literals;

constexpr auto MOVIE_MAGIC = 0x1a34364d;
constexpr auto LATEST_MOVIE_VERSION = 3;
constexpr auto RAWDATA_WARNING_MESSAGE =
    "Warning: One of the active controllers of your input plugin is set to accept \"Raw Data\".\nThis can cause "
    "issues when recording and playing movies. Proceed?";
constexpr auto ROM_NAME_WARNING_MESSAGE = "The movie was recorded on the rom '{}', but is being played back on "
                                          "'{}'.\r\nPlayback might desynchronize. Are you sure you want to continue?";
constexpr auto ROM_COUNTRY_WARNING_MESSAGE = "The movie was recorded on a {} ROM, but is being played back on "
                                             "{}.\r\nPlayback might desynchronize. Are you sure you want to continue?";
constexpr auto ROM_CRC_WARNING_MESSAGE = "The movie was recorded with a ROM that has CRC \"0x{:08X}\",\nbut you are "
                                         "using a ROM with CRC \"0x{:08X}\".\r\nPlayback "
                                         "might desynchronize. Are you sure you want to continue?";
constexpr auto WII_VC_MISMATCH_A_WARNING_MESSAGE =
    "The movie was recorded with WiiVC mode enabled, but is being played back with it disabled.\r\nPlayback might "
    "desynchronize. Are you sure you want to continue?";
constexpr auto WII_VC_MISMATCH_B_WARNING_MESSAGE =
    "The movie was recorded with WiiVC mode disabled, but is being played back with it enabled.\r\nPlayback might "
    "desynchronize. Are you sure you want to continue?";
constexpr auto OLD_MOVIE_EXTENDED_SECTION_NONZERO_MESSAGE =
    "The movie was recorded prior to the extended format being available, but contains data in an extended format "
    "section.\r\nThe movie may be corrupted. Are you sure you want to continue?";
constexpr auto CHEAT_ERROR_ASK_MESSAGE = "This movie has a cheat file associated with it, but it could not be "
                                         "loaded.\r\nPlayback might desynchronize. Are you sure you want to continue?";
constexpr auto CONTROLLER_ON_OFF_MISMATCH =
    "Controller {} is enabled by the input plugin, but it is disabled in the movie.\nPlayback might desynchronize.\n";
constexpr auto CONTROLLER_OFF_ON_MISMATCH =
    "Controller {} is disabled by the input plugin, but it is enabled in the movie.\nPlayback can't commence.\n";
constexpr auto CONTROLLER_MEMPAK_MISMATCH =
    "Controller {} has a Memory Pak in the movie.\nPlayback might desynchronize.\n";
constexpr auto CONTROLLER_RUMBLEPAK_MISMATCH =
    "Controller {} has a Rumble Pak in the movie.\nPlayback might desynchronize.\n";
constexpr auto CONTROLLER_MEMPAK_RUMBLEPAK_MISMATCH =
    "Controller {} does not have a Memory or Rumble Pak in the movie.\nPlayback might desynchronize.\n";

t_vcr_state vcr{};
std::mutex vcr_mtx{};

class vcr_anti_lock
{
  public:
    vcr_anti_lock() { vcr_mtx.unlock(); }

    ~vcr_anti_lock() { vcr_mtx.lock(); }
};

bool vcr_is_task_recording(core_vcr_task task);

bool write_movie_impl(const core_vcr_movie_header *hdr, const std::vector<core_buttons> &inputs,
                      const std::filesystem::path &path)
{
    g_core->log_info(std::format("[VCR] write_movie_impl to {}...", vcr.movie_path.string()));

    core_vcr_movie_header hdr_copy = *hdr;

    if (!g_core->cfg->vcr_write_extended_format)
    {
        g_core->log_info("[VCR] vcr_write_extended_format disabled, replacing new sections with 0...");
        hdr_copy.extended_version = 0;
        memset(&hdr_copy.extended_flags, 0, sizeof(hdr_copy.extended_flags));
        memset(hdr_copy.extended_data.authorship_tag, 0, sizeof(hdr_copy.extended_data.authorship_tag));
        memset(&hdr_copy.extended_data, 0, sizeof(hdr_copy.extended_flags));
    }

    std::vector<uint8_t> out_buf(sizeof(core_vcr_movie_header) + sizeof(core_buttons) * hdr_copy.length_samples);
    std::memcpy(out_buf.data(), &hdr_copy, sizeof(core_vcr_movie_header));
    std::memcpy(out_buf.data() + sizeof(core_vcr_movie_header), inputs.data(),
                sizeof(core_buttons) * hdr_copy.length_samples);
    const auto written = IOUtils::write_entire_file(path, out_buf);

    return written;
}

// Writes the movie header + inputs to current movie_path
bool write_movie()
{
    if (!vcr_is_task_recording(vcr.task))
    {
        g_core->log_info("[VCR] Tried to flush current movie while not in recording task");
        return true;
    }

    g_core->log_info("[VCR] Flushing current movie...");

    return write_movie_impl(&vcr.hdr, vcr.inputs, vcr.movie_path);
}

bool write_backup_impl()
{
    g_core->log_info("[VCR] Backing up movie...");
    const auto filename =
        std::format("{}.{}.m64", vcr.movie_path.stem().string(), static_cast<uint64_t>(time(nullptr)));

    return write_movie_impl(&vcr.hdr, vcr.inputs, g_core->get_backups_directory() / filename);
}

bool is_task_playback(const core_vcr_task task)
{
    return task == task_start_playback_from_reset || task == task_start_playback_from_snapshot || task == task_playback;
}

uint64_t get_rerecord_count()
{
    return static_cast<uint64_t>(vcr.hdr.extended_data.rerecord_count) << 32 | vcr.hdr.rerecord_count;
}

void set_rerecord_count(const uint64_t value)
{
    vcr.hdr.rerecord_count = static_cast<uint32_t>(value & 0xFFFFFFFF);
    vcr.hdr.extended_data.rerecord_count = static_cast<uint32_t>(value >> 32);
}

void execute_post_unlock_callbacks(std::queue<std::function<void()>> &callbacks)
{
    while (!callbacks.empty())
    {
        callbacks.front()();
        callbacks.pop();
    }
}

static std::filesystem::path find_accompanying_file_for_movie(std::filesystem::path path,
                                                              std::span<const std::string_view> extensions)
{
    auto filename = path.filename().generic_string();

    // iterate over every period's position in the string
    std::string st_filename;
    size_t dot_pos = filename.find('.');
    do
    {
        // Standard case, no st sharing
        std::string_view matched = std::string_view(filename).substr(0, dot_pos);

        // try to match against every extension and see if the filename exists
        for (auto ext : extensions)
        {
            st_filename.clear();
            std::format_to(std::back_inserter(st_filename), "{}{}", matched, ext);

            std::filesystem::path st_path = path.replace_filename(st_filename);
            if (std::filesystem::exists(st_path))
            {
                // we have our match
                return st_path;
            }
        }

        dot_pos = filename.find('.', dot_pos + 1);
    } while (dot_pos != std::string::npos);

    // we've tried everything, return the empty path
    return std::filesystem::path();
}

static std::filesystem::path find_accompanying_file_for_movie(const std::filesystem::path &path,
                                                              std::initializer_list<std::string_view> extensions = {
                                                                  ".st"sv, ".savestate"sv})
{
    return find_accompanying_file_for_movie(path,
                                            std::span<const std::string_view>{extensions.begin(), extensions.size()});
}

static void set_rom_info(core_vcr_movie_header *header)
{
    header->vis_per_second = g_ctx.vr_get_vis_per_second(g_ctx.vr_get_rom_header()->Country_code);
    header->controller_flags = 0;
    header->num_controllers = 0;

    for (int32_t i = 0; i < 4; ++i)
    {
        if (g_core->controls[i].Plugin == (int32_t)ce_mempak)
        {
            header->controller_flags |= CONTROLLER_X_MEMPAK(i);
        }

        if (g_core->controls[i].Plugin == (int32_t)ce_rumblepak)
        {
            header->controller_flags |= CONTROLLER_X_RUMBLE(i);
        }

        if (!g_core->controls[i].Present) continue;

        header->controller_flags |= CONTROLLER_X_PRESENT(i);
        header->num_controllers++;
    }

    strncpy(header->rom_name, (const char *)ROM_HEADER.nom, 32);
    header->rom_crc1 = ROM_HEADER.CRC1;
    header->rom_country = ROM_HEADER.Country_code;

    header->input_plugin_name[0] = '\0';
    header->video_plugin_name[0] = '\0';
    header->audio_plugin_name[0] = '\0';
    header->rsp_plugin_name[0] = '\0';

    g_core->get_plugin_names(header->video_plugin_name, header->audio_plugin_name, header->input_plugin_name,
                             header->rsp_plugin_name);
}

core_result vcr_read_movie_header(std::vector<uint8_t> buf, core_vcr_movie_header *header)
{
    const core_vcr_movie_header default_hdr{};
    constexpr auto old_header_size = 512;

    if (buf.size() < old_header_size) return VCR_InvalidFormat;

    core_vcr_movie_header new_header = {};
    memcpy(&new_header, buf.data(), old_header_size);

    if (new_header.magic != MOVIE_MAGIC) return VCR_InvalidFormat;

    if (new_header.version <= 0 || new_header.version > LATEST_MOVIE_VERSION) return VCR_InvalidVersion;

    // The extended version number can't exceed the latest one, obviously...
    if (new_header.extended_version > default_hdr.extended_version) return VCR_InvalidExtendedVersion;

    if (new_header.version == 1 || new_header.version == 2)
    {
        // attempt to recover screwed-up plugin data caused by
        // version mishandling and format problems of first versions

#define IS_ALPHA(x) (((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || ((x) == '1'))
        int32_t i;
        for (i = 0; i < 56 + 64; i++)
            if (IS_ALPHA(new_header.reserved_bytes[i]) && IS_ALPHA(new_header.reserved_bytes[i + 64]) &&
                IS_ALPHA(new_header.reserved_bytes[i + 64 + 64]) &&
                IS_ALPHA(new_header.reserved_bytes[i + 64 + 64 + 64]))
                break;
        if (i != 56 + 64)
        {
            memmove(new_header.video_plugin_name, new_header.reserved_bytes + i, 256);
        }
        else
        {
            for (i = 0; i < 56 + 64; i++)
                if (IS_ALPHA(new_header.reserved_bytes[i]) && IS_ALPHA(new_header.reserved_bytes[i + 64]) &&
                    IS_ALPHA(new_header.reserved_bytes[i + 64 + 64]))
                    break;
            if (i != 56 + 64)
                memmove(new_header.audio_plugin_name, new_header.reserved_bytes + i, 256 - 64);
            else
            {
                for (i = 0; i < 56 + 64; i++)
                    if (IS_ALPHA(new_header.reserved_bytes[i]) && IS_ALPHA(new_header.reserved_bytes[i + 64])) break;
                if (i != 56 + 64)
                    memmove(new_header.input_plugin_name, new_header.reserved_bytes + i, 256 - 64 - 64);
                else
                {
                    for (i = 0; i < 56 + 64; i++)
                        if (IS_ALPHA(new_header.reserved_bytes[i])) break;
                    if (i != 56 + 64)
                        memmove(new_header.rsp_plugin_name, new_header.reserved_bytes + i, 256 - 64 - 64 - 64);
                    else
                        strncpy(new_header.rsp_plugin_name, "(unknown)", 64);

                    strncpy(new_header.input_plugin_name, "(unknown)", 64);
                }
                strncpy(new_header.audio_plugin_name, "(unknown)", 64);
            }
            strncpy(new_header.video_plugin_name, "(unknown)", 64);
        }
        // attempt to convert old author and description to utf8
        strncpy(new_header.author, new_header.old_author_info, 48);
        strncpy(new_header.description, new_header.old_description, 80);
    }
    if (new_header.version == 3 && buf.size() < sizeof(core_vcr_movie_header))
    {
        return VCR_InvalidFormat;
    }
    if (new_header.version == 3)
    {
        memcpy(new_header.author, buf.data() + 0x222, 222);
        memcpy(new_header.description, buf.data() + 0x300, 256);

        // Some movies have a higher length_samples than the actual input buffer size, so we patch the length_samples up
        // and emit a warning
        const auto actual_sample_count = (buf.size() - sizeof(core_vcr_movie_header)) / sizeof(core_buttons);

        if (new_header.length_samples > actual_sample_count)
        {
            g_core->log_warn(std::format("[VCR] Header has length_samples of {}, but the actual input buffer size is "
                                         "{}. Clamping length_samples...",
                                         new_header.length_samples, actual_sample_count));
            new_header.length_samples = actual_sample_count;
        }
    }

    *header = new_header;

    return Res_Ok;
}

core_result vcr_parse_header(std::filesystem::path path, core_vcr_movie_header *header)
{
    if (path.extension().compare(L".m64") != 0)
    {
        return VCR_InvalidFormat;
    }

    core_vcr_movie_header new_header = {};
    new_header.rom_country = -1;
    strncpy(new_header.rom_name, "(no ROM)", sizeof(new_header.rom_name));

    auto buf = IOUtils::read_entire_file(path);
    if (buf.empty())
    {
        return VCR_BadFile;
    }

    const auto result = vcr_read_movie_header(buf, &new_header);
    *header = new_header;

    return result;
}

core_result vcr_read_movie_inputs(std::filesystem::path path, std::vector<core_buttons> &inputs)
{
    core_vcr_movie_header header = {};
    const auto result = vcr_parse_header(path, &header);
    if (result != Res_Ok)
    {
        return result;
    }

    auto buf = IOUtils::read_entire_file(path);

    if (buf.size() < sizeof(core_vcr_movie_header) + sizeof(core_buttons) * header.length_samples)
    {
        return VCR_InvalidFormat;
    }

    inputs.resize(header.length_samples);
    memcpy(inputs.data(), buf.data() + sizeof(core_vcr_movie_header), sizeof(core_buttons) * header.length_samples);

    return Res_Ok;
}

void vcr_increment_rerecord_count()
{
    if (vcr.task != task_recording)
    {
        return;
    }

    set_rerecord_count(get_rerecord_count() + 1);
    g_core->cfg->total_rerecords++;
}

bool vcr_freeze(vcr_freeze_info &freeze)
{
    constexpr size_t FREEZE_MAX_SIZE = (UINT32_MAX - 20) / 4;

    std::unique_lock lock(vcr_mtx);

    if (vcr.task == task_idle)
    {
        return false;
    }

    assert(vcr.inputs.size() >= vcr.hdr.length_samples);
    assert(vcr.hdr.length_samples <= FREEZE_MAX_SIZE); // safety check

    freeze = {
        .size = static_cast<uint32_t>(sizeof(uint32_t) * 4 + sizeof(core_buttons) * (vcr.hdr.length_samples + 1)),
        .uid = vcr.hdr.uid,
        .current_sample = (uint32_t)vcr.current_sample,
        .current_vi = (uint32_t)vcr.current_vi,
        .length_samples = vcr.hdr.length_samples,
    };

    // NOTE: The frozen input buffer is weird: its length is traditionally equal to length_samples + 1, which means the
    // last frame is garbage data
    freeze.input_buffer = {};
    freeze.input_buffer.resize(vcr.hdr.length_samples + 1);
    memcpy(freeze.input_buffer.data(), vcr.inputs.data(), sizeof(core_buttons) * vcr.hdr.length_samples);

    // Also probably a good time to flush the movie
    write_movie();

    return true;
}

core_result vcr_unfreeze(const vcr_freeze_info &freeze)
{
    std::unique_lock lock(vcr_mtx);

    // Unfreezing isn't valid during idle state
    if (vcr.task == task_idle)
    {
        return VCR_NeedsPlaybackOrRecording;
    }

    if (freeze.size <
        sizeof(vcr.hdr.uid) + sizeof(vcr.current_sample) + sizeof(vcr.current_vi) + sizeof(vcr.hdr.length_samples))
    {
        return VCR_InvalidFormat;
    }

    const uint32_t space_needed = sizeof(core_buttons) * (freeze.length_samples + 1);

    if (freeze.uid != vcr.hdr.uid) return VCR_NotFromThisMovie;

    // This means playback desync in read-only mode, but in read-write mode it's fine, as the input buffer will be
    // copied and grown from st.
    if (freeze.current_sample > freeze.length_samples && g_core->cfg->vcr_readonly) return VCR_InvalidFrame;

    if (space_needed > freeze.size) return VCR_InvalidFormat;

    vcr.current_sample = (int32_t)freeze.current_sample;
    vcr.current_vi = (int32_t)freeze.current_vi;

    const core_vcr_task last_task = vcr.task;

    // When starting playback in RW mode, we don't want overwrite the movie savestate which we're currently unfreezing
    // from...
    const bool is_task_starting_playback =
        vcr.task == task_start_playback_from_reset || vcr.task == task_start_playback_from_snapshot;

    // When unfreezing during a seek while recording, we don't want to overwrite the input buffer.
    // Instead, we'll just update the current sample.
    if (vcr.task == task_recording && vcr.seek_to_frame.has_value())
    {
        goto finish;
    }

    if (!g_core->cfg->vcr_readonly && !is_task_starting_playback)
    {
        // here, we are going to take the input data from the savestate
        // and make it the input data for the current movie, then continue
        // writing new input data at the currentFrame pointer
        vcr.task = task_recording;

        // update header with new ROM info
        if (last_task == task_playback) set_rom_info(&vcr.hdr);

        vcr_increment_rerecord_count();

        if (!vcr.warp_modify_active)
        {
            vcr.hdr.length_samples = freeze.current_sample;

            // Before overwriting the input buffer, save a backup
            if (g_core->cfg->vcr_backups)
            {
                write_backup_impl();
            }

            vcr.inputs.resize(freeze.current_sample);
            memcpy(vcr.inputs.data(), freeze.input_buffer.data(), sizeof(core_buttons) * freeze.current_sample);

            write_movie();
        }
    }
    else
    {
        // here, we are going to keep the input data from the movie file
        // and simply rewind to the currentFrame pointer
        // this will cause a desync if the savestate is not in sync
        // with the on-disk recording data, but it's easily solved
        // by loading another savestate or playing the movie from the beginning
        write_movie();
        vcr.task = task_playback;
    }

finish: {
    vcr_anti_lock bypass;
    g_core->callbacks.task_changed(vcr.task);
    g_core->callbacks.current_sample_changed(vcr.current_sample);
    g_core->callbacks.rerecords_changed(get_rerecord_count());
    g_core->callbacks.frame();
    g_core->callbacks.unfreeze_completed();
}
    return Res_Ok;
}

core_result vcr_write_backup()
{
    const auto result = write_backup_impl();
    return result ? Res_Ok : VCR_BadFile;
}

void vcr_create_n_frame_savestate(size_t frame)
{
    assert(vcr.current_sample == frame);

    // OPTIMIZATION: When seeking, we can skip creating seek savestates until near the end where we know they wont be
    // purged
    if (vcr.seek_to_frame.has_value())
    {
        const auto frames_from_end_where_savestates_start_appearing =
            g_core->cfg->seek_savestate_interval * g_core->cfg->seek_savestate_max_count;

        if (vcr.seek_to_frame.value() - vcr.current_sample > frames_from_end_where_savestates_start_appearing)
        {
            g_core->log_info("[VCR] Omitting creation of seek savestate because distance to seek end is big enough");
            return;
        }
    }

    // If our seek savestate map is getting too large, we'll start purging the oldest ones (but not the first one!!!)
    if (vcr.seek_savestates.size() > g_core->cfg->seek_savestate_max_count)
    {
        for (int32_t i = 1; i < vcr.hdr.length_samples; ++i)
        {
            if (vcr.seek_savestates.contains(i))
            {
                g_core->log_info(std::format("[VCR] Map too large! Purging seek savestate at frame {}...", i));
                vcr.seek_savestates.erase(i);
                vcr.post_controller_poll_callbacks.emplace(
                    [=] { g_core->callbacks.seek_savestate_changed((size_t)i); });
                break;
            }
        }
    }

    g_core->log_info(std::format("[VCR] Creating seek savestate at frame {}...", frame));
    g_ctx.st_do_memory(
        {}, core_st_job_save,
        [frame](const core_st_callback_info &info, const auto &buf) {
            std::unique_lock lock(vcr_mtx);

            if (info.result != Res_Ok)
            {
                g_core->show_dialog(std::format("Failed to save seek savestate at frame {}.", frame).c_str(), "VCR",
                                    fsvc_error);
                return;
            }

            g_core->log_info(std::format("[VCR] Seek savestate at frame {} of size {} completed", frame, buf.size()));
            vcr.seek_savestates[frame] = buf;

            {
                vcr_anti_lock bypass;
                g_core->callbacks.seek_savestate_changed(frame);
            }
        },
        false);
}

void vcr_handle_starting_tasks(int32_t index, core_buttons *input)
{
    if (vcr.task == task_start_recording_from_reset)
    {
        bool clear_eeprom = !(vcr.hdr.startFlags & MOVIE_START_FROM_EEPROM);
        vcr.reset_pending = true;
        g_core->submit_task([clear_eeprom] {
            const auto result = vr_reset_rom(clear_eeprom, false);

            std::unique_lock lock(vcr_mtx);
            vcr.reset_pending = false;

            if (result != Res_Ok)
            {
                g_core->show_dialog(
                    "Failed to reset the rom when initiating a from-start recording.\nRecording will be stopped.",
                    "VCR", fsvc_error);
                {
                    vcr_anti_lock bypass;
                    g_ctx.vcr_stop_all();
                }
                return;
            }

            vcr.current_sample = 0;
            vcr.current_vi = 0;
            vcr.task = task_recording;

            {
                vcr_anti_lock bypass;
                g_core->callbacks.task_changed(vcr.task);
                g_core->callbacks.current_sample_changed(vcr.current_sample);
                g_core->callbacks.rerecords_changed(get_rerecord_count());
            }
        });
    }

    if (vcr.task == task_start_playback_from_reset)
    {
        bool clear_eeprom = !(vcr.hdr.startFlags & MOVIE_START_FROM_EEPROM);
        vcr.reset_pending = true;
        g_core->submit_task([clear_eeprom] {
            const auto result = vr_reset_rom(clear_eeprom, false);

            std::unique_lock lock(vcr_mtx);
            vcr.reset_pending = false;

            if (result != Res_Ok)
            {
                g_core->show_dialog(
                    "Failed to reset the rom when playing back a from-start movie.\nPlayback will be stopped.", "VCR",
                    fsvc_error);
                {
                    vcr_anti_lock bypass;
                    g_ctx.vcr_stop_all();
                }
                return;
            }

            vcr.current_sample = 0;
            vcr.current_vi = 0;
            vcr.task = task_playback;

            {
                vcr_anti_lock bypass;
                g_core->callbacks.task_changed(vcr.task);
                g_core->callbacks.current_sample_changed(vcr.current_sample);
                g_core->callbacks.rerecords_changed(get_rerecord_count());
            }
        });
    }
}

void vcr_handle_recording(int32_t index, core_buttons *input)
{
    if (vcr.task != task_recording)
    {
        return;
    }

    // When the movie has more frames after the current one than the buffer has, we need to use the buffer data instead
    // of the plugin
    const auto effective_index = vcr.current_sample + index;
    bool use_inputs_from_buffer = vcr.inputs.size() > effective_index || vcr.warp_modify_active;

    // Regular recording: the recording input source is the input plugin (along with the reset override)
    if (vcr.reset_requested)
    {
        *input = {
            .reserved_1 = 1,
            .reserved_2 = 1,
        };
    }
    else
    {
        if (use_inputs_from_buffer)
        {
            *input = vcr.inputs[effective_index];
            auto dummy_input = *input;

            {
                vcr_anti_lock bypass;
                g_core->callbacks.input(&dummy_input, index);
            }
        }
        else
        {
            g_core->input_get_keys(index, input);

            {
                vcr_anti_lock bypass;
                g_core->callbacks.input(input, index);
            }
        }
    }

    // The VCR state might have changed while the mutex was unlocked. Account for that here.
    // FIXME: Is this enough?
    if (vcr.task != task_recording)
    {
        return;
    }

    if (!use_inputs_from_buffer)
    {
        vcr.inputs.push_back(*input);
        vcr.hdr.length_samples++;
    }

    vcr.current_sample++;

    if (vcr.reset_requested)
    {
        vcr.reset_requested = false;
        vcr.reset_pending = true;
        g_core->submit_task([] {
            core_result result;
            {
                std::lock_guard lock(g_emu_cs);
                result = vr_reset_rom_impl(false, false, true);
            }

            std::unique_lock lock(vcr_mtx);

            vcr.reset_pending = false;

            if (result != Res_Ok)
            {
                g_core->show_dialog("Failed to reset the rom following a user-invoked reset.", "VCR", fsvc_error);
            }
        });
    }

    vcr.post_controller_poll_callbacks.emplace([=] { g_core->callbacks.current_sample_changed(vcr.current_sample); });
}

void vcr_handle_playback(int32_t index, core_buttons *input)
{
    if (vcr.task != task_playback)
    {
        return;
    }

    if (g_core->cfg->wait_at_movie_end && vcr.current_sample == (int32_t)vcr.hdr.length_samples - 1)
    {
        vcr_anti_lock bypass;
        g_ctx.vr_pause_emu();
    }

    // This if previously also checked for if the VI is over the amount specified in the header,
    // but that can cause movies to end playback early on laggy plugins.
    if (vcr.current_sample >= (int32_t)vcr.hdr.length_samples)
    {
        {
            vcr_anti_lock bypass;
            g_ctx.vcr_stop_all();
        }

        if (g_core->cfg->is_movie_loop_enabled)
        {
            {
                vcr_anti_lock bypass;
                g_ctx.vcr_start_playback(vcr.movie_path);
            }

            vcr.post_controller_poll_callbacks.emplace([=] { g_core->callbacks.loop_movie(); });
            return;
        }

        g_core->input_set_keys(index, {0});
        g_core->input_get_keys(index, input);
        return;
    }

    if (!(vcr.hdr.controller_flags & CONTROLLER_X_PRESENT(index)))
    {
        // disconnected controls are forced to have no input during playback
        *input = {0};
        return;
    }

    // Use inputs from movie, also notify input plugin of override via setKeys
    *input = vcr.inputs[vcr.current_sample];

    // no readable code because 120 star tas can't get this right >:(
    if (input->value == 0xC000)
    {
        vcr.reset_pending = true;
        g_core->log_info("[VCR] Resetting during playback...");
        g_core->submit_task([] {
            auto result = vr_reset_rom(false, false);

            std::unique_lock lock(vcr_mtx);

            if (result != Res_Ok)
            {
                g_core->show_dialog(
                    "Failed to reset the rom following a movie-invoked reset.\nRecording will be stopped.", "VCR",
                    fsvc_error);
                {
                    vcr_anti_lock bypass;
                    g_ctx.vcr_stop_all();
                }
                vcr.reset_pending = false;
                return;
            }

            vcr.reset_pending = false;
        });
    }

    g_core->input_set_keys(index, *input);

    {
        vcr_anti_lock bypass;
        g_core->callbacks.input(input, index);
    }
    // We don't need to account for state changes during the unlocked period here, as we don't do any more immediate
    // state-dependent work.

    vcr.current_sample++;
    vcr.post_controller_poll_callbacks.emplace([=] { g_core->callbacks.current_sample_changed(vcr.current_sample); });
}

void vcr_stop_seek_if_needed()
{
    if (!vcr.seek_to_frame.has_value())
    {
        return;
    }

    assert(vcr.task != task_idle);

    if (vcr.current_sample > vcr.seek_to_frame.value())
    {
        g_core->show_dialog("Seek frame exceeded without seek having been stopped!\nThis incident has been logged, "
                            "please report this issue along with the log file.",
                            "VCR", fsvc_error);
    }

    if (vcr.current_sample >= vcr.seek_to_frame.value())
    {
        g_core->log_info(
            std::format("[VCR] Seek finished at frame {} (target: {})", vcr.current_sample, vcr.seek_to_frame.value()));

        {
            vcr_anti_lock bypass;
            g_ctx.vcr_stop_seek();
        }

        if (vcr.seek_pause_at_end)
        {
            vcr_anti_lock bypass;
            g_ctx.vr_pause_emu();
        }
    }
}

bool vcr_allows_core_pause()
{
    if (!vcr.seek_to_frame.has_value())
    {
        return true;
    }

    return vcr.seek_pause_at_end && vcr.current_sample == vcr.seek_to_frame.value() - 1;
}

bool vcr_allows_core_unpause()
{
    if (g_core->cfg->wait_at_movie_end && vcr.task == task_playback && vcr.current_sample >= vcr.hdr.length_samples - 1)
    {
        return false;
    }
    return true;
}

void vcr_request_reset()
{
    g_core->log_trace("vr_reset_rom_impl Reset during recording, handing off to VCR");
    vcr.reset_requested = true;
}

void vcr_create_seek_savestates()
{
    if (vcr.task == task_idle || g_core->cfg->seek_savestate_interval == 0)
    {
        return;
    }

    if (vcr.current_sample % g_core->cfg->seek_savestate_interval == 0)
    {
        vcr_create_n_frame_savestate(vcr.current_sample);
    }
}

void vcr_on_controller_poll(int32_t index, core_buttons *input)
{
    std::unique_lock lock(vcr_mtx);

    // NOTE: When we call reset_rom from another thread, we only request a reset to happen in the future.
    // Until the reset, the emu thread keeps running and potentially generating many frames.
    // Those frames are invalid to us, because from the movie's perspective, it should be instantaneous.
    if (vcr.reset_pending)
    {
        g_core->log_info("[VCR] Skipping pre-reset frame");
        return;
    }

    // Frames between seek savestate load request and actual load are invalid for the same reason as pre-reset frames.
    if (vcr.seek_savestate_loading)
    {
        g_core->log_info("[VCR] Skipping pre-seek savestate load frame");
        return;
    }

    if (vcr.task == task_idle)
    {
        g_core->input_get_keys(index, input);

        {
            vcr_anti_lock bypass;
            g_core->callbacks.input(input, index);
        }

        return;
    }

    vcr_stop_seek_if_needed();

    // We need to handle start tasks first, as logic after it depends on the task being modified
    vcr_handle_starting_tasks(index, input);

    vcr_create_seek_savestates();

    vcr_handle_recording(index, input);

    vcr_handle_playback(index, input);

    // Since the callback might want to call VCR functions, we have to release the lock to avoid deadlocking in
    // situations with interlocked threads (e.g. UI and Emu) In addition, we have to be careful to only call this
    // function after we're done with VCR work as to avoid reentrancy issues.
    {
        vcr_anti_lock bypass;
        while (!vcr.post_controller_poll_callbacks.empty())
        {
            vcr.post_controller_poll_callbacks.front()();
            vcr.post_controller_poll_callbacks.pop();
        }
    }
}

// Generates a savestate path for a newly created movie.
// Consists of the movie path, but with the stem trimmed at the first dot and with the specified extension (must contain
// dot)
std::filesystem::path get_path_for_new_movie(std::filesystem::path path, std::string_view extension = ".st"sv)
{
    auto stem = path.stem().generic_string();
    auto stem_dot_pos = stem.find('.');

    // Standard case, no st shortcutting
    if (stem_dot_pos == std::string::npos)
    {
        path.replace_extension(extension);
        return path;
    }

    // Trim the stem to the first dot, edit the filename
    stem.resize(stem_dot_pos);
    path.replace_filename(std::format("{}{}", stem, extension));
    return path;
}

core_result vcr_start_record(std::filesystem::path path, uint16_t flags, std::string author, std::string description)
{
    std::unique_lock lock(vcr_mtx);

    if (flags != MOVIE_START_FROM_SNAPSHOT && flags != MOVIE_START_FROM_NOTHING && flags != MOVIE_START_FROM_EEPROM)
    {
        return VCR_InvalidStartType;
    }

    if (author.empty())
    {
        author = "(no author)";
    }

    if (description.empty())
    {
        description = "(no description)";
    }

    const auto cheat_data = cht_serialize();
    if (!cheat_data.empty())
    {
        const auto cheat_path = get_path_for_new_movie(path, ".cht");
        g_core->log_info(std::format("Writing movie cheat data to {}...", cheat_path.string()));

        std::ofstream file(cheat_path, std::ios::out);
        if (!file)
        {
            g_core->log_error("core_vcr_start_record cheat std::wofstream failed");
            return VCR_CheatWriteFailed;
        }
        file << cheat_data;
        if (file.fail())
        {
            g_core->log_error("core_vcr_start_record cheat write failed");
            return VCR_CheatWriteFailed;
        }
        file.close();
        if (file.bad())
        {
            g_core->log_error("core_vcr_start_record file bad");
            return VCR_CheatWriteFailed;
        }
    }

    {
        vcr_anti_lock bypass;
        g_ctx.vcr_stop_all();
    }
    vcr.movie_path = path;

    for (auto &[Present, RawData, Plugin] : g_core->controls)
    {
        if (Present && RawData)
        {
            bool proceed = g_core->show_ask_dialog(CORE_DLG_VCR_RAWDATA_WARNING, RAWDATA_WARNING_MESSAGE, "VCR", true);
            if (!proceed)
            {
                return Res_Cancelled;
            }
            break;
        }
    }

    // FIXME: Do we want to reset this every time?
    g_core->cfg->vcr_readonly = 0;

    const core_vcr_movie_header default_hdr{};
    memset(&vcr.hdr, 0, sizeof(core_vcr_movie_header));
    vcr.inputs = {};

    vcr.hdr.magic = MOVIE_MAGIC;
    vcr.hdr.version = LATEST_MOVIE_VERSION;

    vcr.hdr.extended_version = default_hdr.extended_version;
    vcr.hdr.extended_flags.wii_vc = g_core->cfg->wii_vc_emulation;
    vcr.hdr.extended_data = default_hdr.extended_data;

    vcr.hdr.uid = (uint32_t)time(nullptr);
    vcr.hdr.length_vis = 0;
    vcr.hdr.length_samples = 0;

    set_rerecord_count(0);
    vcr.hdr.startFlags = flags;

    if (flags & MOVIE_START_FROM_SNAPSHOT)
    {
        // save state
        g_core->log_info("[VCR] Saving state...");
        vcr.task = task_start_recording_from_snapshot;
        g_ctx.st_do_file(
            get_path_for_new_movie(vcr.movie_path), core_st_job_save,
            [](const core_st_callback_info &info, auto &&...) {
                std::unique_lock lock(vcr_mtx);

                if (info.result != Res_Ok)
                {
                    g_core->show_dialog(
                        "Failed to save savestate while starting recording.\nRecording will be stopped.", "VCR",
                        fsvc_error);

                    {
                        vcr_anti_lock bypass;
                        g_ctx.vcr_stop_all();
                    }

                    return;
                }

                g_core->log_info("[VCR] Starting recording from snapshot...");
                vcr.task = task_recording;
                // FIXME: Doesn't this need a message broadcast?
                // TODO: Also, what about clearing the input on first frame
            },
            true);
    }
    else
    {
        vcr.task = task_start_recording_from_reset;
    }

    set_rom_info(&vcr.hdr);

    memset(vcr.hdr.author, 0, sizeof(core_vcr_movie_header::author));
    if (author.size() > sizeof(core_vcr_movie_header::author))
    {
        author.resize(sizeof(core_vcr_movie_header::author));
    }
    author.copy(vcr.hdr.author, sizeof(vcr.hdr.author));
    // strncpy_s(vcr.hdr.author, sizeof(vcr.hdr.author), author.data(), author.size());

    memset(vcr.hdr.description, 0, sizeof(core_vcr_movie_header::description));
    if (description.size() > sizeof(core_vcr_movie_header::description))
    {
        description.resize(sizeof(core_vcr_movie_header::description));
    }
    description.copy(vcr.hdr.description, sizeof(vcr.hdr.description));
    // strncpy_s(vcr.hdr.description, sizeof(vcr.hdr.description), description.data(), description.size());

    vcr.current_sample = 0;
    vcr.current_vi = 0;

    {
        vcr_anti_lock bypass;
        g_core->callbacks.task_changed(vcr.task);
        g_core->callbacks.current_sample_changed(vcr.current_sample);
        g_core->callbacks.rerecords_changed(get_rerecord_count());
        g_core->callbacks.readonly_changed((bool)g_core->cfg->vcr_readonly);
    }

    return Res_Ok;
}

core_result vcr_continue_recording()
{
    std::unique_lock lock(vcr_mtx);

    if (vcr.task != task_playback && vcr.task != task_start_playback_from_reset &&
        vcr.task != task_start_playback_from_snapshot)
    {
        return VCR_NeedsPlayback;
    }

    if (g_core->cfg->vcr_backups)
    {
        write_backup_impl();
    }

    vcr.task = task_recording;
    vcr.inputs.resize(vcr.current_sample);
    vcr.hdr.length_samples = vcr.inputs.size();
    vcr.hdr.length_vis = vcr.current_vi;
    set_rom_info(&vcr.hdr);
    write_movie();

    {
        vcr_anti_lock bypass;
        g_core->callbacks.task_changed(vcr.task);
    }

    return Res_Ok;
}

core_result vcr_replace_author_info(const std::filesystem::path &path, const std::string &author,
                                    const std::string &description)
{
    // 0. validate lengths
    if (author.size() > 222 || description.size() > 256)
    {
        return VCR_InvalidFormat;
    }

    // 1. Read movie header
    const auto buf = IOUtils::read_entire_file(path);

    if (buf.empty())
    {
        return VCR_BadFile;
    }

    core_vcr_movie_header hdr{};
    auto result = vcr_read_movie_header(buf, &hdr);

    if (result != Res_Ok)
    {
        return result;
    }

    // 2. Compare author and description fields, and don't do any work if they remained identical
    if (!strcmp(hdr.author, author.c_str()) && !strcmp(hdr.description, description.c_str()))
    {
        g_core->log_info("[VCR] Movie author or description didn't change, returning early...");
        return Res_Ok;
    }

    // 3. prepare padded buffers for output
    std::string author_out;
    author_out.reserve(222);
    author_out.assign(author);
    author_out.resize(222, '\0');

    std::string description_out;
    description_out.reserve(256);
    description_out.assign(description);
    description_out.resize(256, '\0');

    // 4. actually write the file
    {
        std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            return VCR_BadFile;
        }

        file.seekg(0x222, std::ios::beg);
        file.write(author_out.data(), author_out.size());

        file.seekg(0x256, std::ios::beg);
        file.write(description_out.data(), description_out.size());
    }

    return Res_Ok;
}

core_vcr_seek_info vcr_get_seek_info()
{
    std::unique_lock lock(vcr_mtx);

    core_vcr_seek_info info{};

    info.current_sample = vcr.current_sample;
    info.seek_start_sample = vcr.seek_to_frame.has_value() ? vcr.seek_start_sample : SIZE_MAX;
    info.seek_target_sample = vcr.seek_to_frame.value_or(SIZE_MAX);

    return info;
}

bool show_controller_warning(const core_vcr_movie_header &header)
{
    for (int32_t i = 0; i < 4; ++i)
    {
        if (!g_core->controls[i].Present && header.controller_flags & CONTROLLER_X_PRESENT(i))
        {
            g_core->show_dialog(std::format(CONTROLLER_OFF_ON_MISMATCH, i + 1).c_str(), "VCR", fsvc_error);
            return false;
        }
        if (g_core->controls[i].Present && !(header.controller_flags & CONTROLLER_X_PRESENT(i)))
        {
            g_core->show_dialog(std::format(CONTROLLER_ON_OFF_MISMATCH, i + 1).c_str(), "VCR", fsvc_warning);
        }
        else
        {
            if (g_core->controls[i].Present && (g_core->controls[i].Plugin != (int32_t)ce_mempak) &&
                header.controller_flags & CONTROLLER_X_MEMPAK(i))
            {
                g_core->show_dialog(std::format(CONTROLLER_MEMPAK_MISMATCH, i + 1).c_str(), "VCR", fsvc_warning);
            }
            if (g_core->controls[i].Present && (g_core->controls[i].Plugin != (int32_t)ce_rumblepak) &&
                header.controller_flags & CONTROLLER_X_RUMBLE(i))
            {
                g_core->show_dialog(std::format(CONTROLLER_RUMBLEPAK_MISMATCH, i + 1).c_str(), "VCR", fsvc_warning);
            }
            if (g_core->controls[i].Present && (g_core->controls[i].Plugin != (int32_t)ce_none) &&
                !(header.controller_flags & (CONTROLLER_X_MEMPAK(i) | CONTROLLER_X_RUMBLE(i))))
            {
                g_core->show_dialog(std::format(CONTROLLER_MEMPAK_RUMBLEPAK_MISMATCH, i + 1).c_str(), "VCR",
                                    fsvc_warning);
            }
        }
    }
    return true;
}

core_result vcr_start_playback(std::filesystem::path path)
{
    std::unique_lock lock(vcr_mtx);

    auto movie_buf = IOUtils::read_entire_file(path);

    if (movie_buf.empty())
    {
        return VCR_BadFile;
    }

    if (!core_executing)
    {
        // NOTE: We need to unlock the VCR mutex during the vr_start_rom call, as it needs the core to continue
        // execution in order to exit. If we kept the lock, the core would become permanently stuck waiting for it to be
        // released in on_controller_poll.
        {
            vcr_anti_lock bypass;
            const auto result = g_ctx.vr_start_rom(path);

            if (result != Res_Ok)
            {
                return result;
            }
        }
    }

    core_vcr_movie_header header{};
    const auto result = vcr_read_movie_header(movie_buf, &header);
    if (result != Res_Ok)
    {
        return result;
    }

    std::vector<core_buttons> movie_inputs{};
    movie_inputs.resize(header.length_samples);
    memcpy(movie_inputs.data(), movie_buf.data() + sizeof(core_vcr_movie_header),
           sizeof(core_buttons) * header.length_samples);

    for (auto &[Present, RawData, Plugin] : g_core->controls)
    {
        if (!Present || !RawData) continue;

        bool proceed = g_core->show_ask_dialog(CORE_DLG_VCR_RAWDATA_WARNING, RAWDATA_WARNING_MESSAGE, "VCR", true);
        if (!proceed)
        {
            return Res_Cancelled;
        }

        break;
    }

    if (!show_controller_warning(header))
    {
        return VCR_InvalidControllers;
    }

    if (header.extended_version != 0)
    {
        g_core->log_info(std::format("[VCR] Movie has extended version {}", header.extended_version));

        if ((bool)g_core->cfg->wii_vc_emulation != header.extended_flags.wii_vc)
        {
            bool proceed = g_core->show_ask_dialog(CORE_DLG_VCR_WIIVC_WARNING,
                                                   header.extended_flags.wii_vc ? WII_VC_MISMATCH_A_WARNING_MESSAGE
                                                                                : WII_VC_MISMATCH_B_WARNING_MESSAGE,
                                                   "VCR", true);

            if (!proceed)
            {
                return Res_Cancelled;
            }
        }
    }
    else
    {
        // Old movies filled with non-zero data in this section are suspicious, we'll warn the user.
        if (header.extended_flags.data != 0)
        {
            g_core->show_dialog(OLD_MOVIE_EXTENDED_SECTION_NONZERO_MESSAGE, "VCR", fsvc_warning);
        }
    }

    if (StrUtils::c_icmp(header.rom_name, (const char *)ROM_HEADER.nom) != 0)
    {
        bool proceed = g_core->show_ask_dialog(
            CORE_DLG_VCR_ROM_NAME_WARNING,
            std::format(ROM_NAME_WARNING_MESSAGE, header.rom_name, (char *)ROM_HEADER.nom).c_str(), "VCR", true);

        if (!proceed)
        {
            return Res_Cancelled;
        }
    }
    else
    {
        if (header.rom_country != ROM_HEADER.Country_code)
        {
            bool proceed = g_core->show_ask_dialog(
                CORE_DLG_VCR_ROM_CCODE_WARNING,
                std::format(ROM_COUNTRY_WARNING_MESSAGE, g_ctx.vr_country_code_to_country_name(header.rom_country),
                            g_ctx.vr_country_code_to_country_name(ROM_HEADER.Country_code))
                    .c_str(),
                "VCR", true);

            if (!proceed)
            {
                return Res_Cancelled;
            }
        }
        else if (header.rom_crc1 != ROM_HEADER.CRC1)
        {
            // wchar_t str[512] = {0};
            // swprintf_s(str, ROM_CRC_WARNING_MESSAGE, header.rom_crc1, ROM_HEADER.CRC1);
            auto ask_message = std::format(ROM_CRC_WARNING_MESSAGE, header.rom_crc1, ROM_HEADER.CRC1);

            bool proceed = g_core->show_ask_dialog(CORE_DLG_VCR_ROM_CRC_WARNING, ask_message.c_str(), "VCR", true);
            if (!proceed)
            {
                return Res_Cancelled;
            }
        }
    }

    const auto cht_path = find_accompanying_file_for_movie(path, {".cht"});

    if (!cht_path.empty())
    {
        std::vector<core_cheat> cheats;
        if (cht_read_from_file(cht_path, cheats))
        {
            cht_layer_push(cheats);
        }
        else
        {
            const auto proceed =
                g_core->show_ask_dialog(CORE_DLG_VCR_CHEAT_LOAD_ERROR, CHEAT_ERROR_ASK_MESSAGE, "VCR", true);

            if (!proceed)
            {
                return Res_Cancelled;
            }
        }
    }
    else
    {
        // Push a fake empty layer to ensure that the potentially active cheats are overriden by nothing
        cht_layer_push({});
    }

    {
        vcr_anti_lock bypass;
        g_ctx.vcr_stop_all();
    }

    vcr.current_sample = 0;
    vcr.current_vi = 0;
    vcr.movie_path = path;
    vcr.inputs = movie_inputs;
    vcr.hdr = header;

    if (header.startFlags & MOVIE_START_FROM_SNAPSHOT)
    {
        g_core->log_info("[VCR] Loading state...");

        // Load appropriate state for movie
        auto st_path = find_accompanying_file_for_movie(vcr.movie_path);

        if (st_path.empty())
        {
            return VCR_InvalidSavestate;
        }

        vcr.task = task_start_playback_from_snapshot;

        g_core->submit_task([=] {
            g_ctx.st_do_file(
                st_path, core_st_job_load,
                [](const core_st_callback_info &info, auto &&...) {
                    std::unique_lock lock(vcr_mtx);

                    if (info.result != Res_Ok)
                    {
                        g_core->show_dialog(
                            "Failed to load savestate while starting playback.\nRecording will be stopped.", "VCR",
                            fsvc_error);

                        {
                            vcr_anti_lock bypass;
                            g_ctx.vcr_stop_all();
                        }

                        return;
                    }

                    g_core->log_info("[VCR] Starting playback from snapshot...");
                    vcr.task = task_playback;

                    {
                        vcr_anti_lock bypass;
                        g_core->callbacks.task_changed(vcr.task);
                        g_core->callbacks.current_sample_changed(vcr.current_sample);
                        g_core->callbacks.rerecords_changed(get_rerecord_count());
                    }
                },
                true);
        });
    }
    else
    {
        vcr.task = task_start_playback_from_reset;
    }

    {
        vcr_anti_lock bypass;
        g_core->callbacks.task_changed(vcr.task);
        g_core->callbacks.current_sample_changed(vcr.current_sample);
        g_core->callbacks.rerecords_changed(get_rerecord_count());
        g_core->callbacks.play_movie();
    }

    return Res_Ok;
}

static bool can_seek_to(size_t frame)
{
    return frame <= vcr.hdr.length_samples && frame > 0;
}

static size_t compute_sample_from_seek_string(const std::string &str)
{
    try
    {
        if (str[0] == '-' || str[0] == '+')
        {
            return vcr.current_sample + std::stoi(str);
        }

        if (str[0] == '^')
        {
            return vcr.hdr.length_samples - std::stoi(str.substr(1));
        }

        return std::stoi(str);
    }
    catch (...)
    {
        return SIZE_MAX;
    }
}

size_t vcr_find_closest_savestate_before_frame(size_t frame)
{
    int32_t lowest_distance = INT32_MAX;
    size_t lowest_distance_frame = 0;
    for (const auto &[slot_frame, _] : vcr.seek_savestates)
    {
        // Current and future sts are invalid for rewinding
        if (slot_frame >= frame)
        {
            continue;
        }

        auto distance = frame - slot_frame;

        if (distance < lowest_distance)
        {
            lowest_distance = distance;
            lowest_distance_frame = slot_frame;
        }
    }

    return lowest_distance_frame;
}

static core_result vcr_begin_seek_impl(std::string str, bool pause_at_end, bool resume, bool warp_modify)
{
    std::unique_lock lock(vcr_mtx);

    // Queue of functions to call at the end of the function after the lock is released
    std::queue<std::function<void()>> post_unlock_callbacks{};

    if (vcr.seek_savestate_loading || vcr.seek_to_frame.has_value())
    {
        return VCR_SeekAlreadyRunning;
    }

    if (vcr.task == task_idle)
    {
        return VCR_Idle;
    }

    auto frame = compute_sample_from_seek_string(str);

    if (frame == SIZE_MAX || !can_seek_to(frame))
    {
        return VCR_InvalidFrame;
    }

    // We need to adjust the end frame if we're pausing at the end... lol
    if (pause_at_end)
    {
        frame--;

        if (!can_seek_to(frame))
        {
            return VCR_InvalidFrame;
        }
    }

    vcr.seek_to_frame = std::make_optional(frame);
    vcr.seek_pause_at_end = pause_at_end;

    if (!warp_modify && pause_at_end && vcr.current_sample == frame + 1)
    {
        g_core->log_trace(std::format("[VCR] Early-stopping seek: already at frame {}.", frame));

        {
            vcr_anti_lock bypass;
            g_ctx.vcr_stop_seek();
        }

        return Res_Ok;
    }

    if (resume)
    {
        g_ctx.vr_resume_emu();
    }

    // Fast path: No backtracking required, just continue ahead
    if (vcr.current_sample <= frame)
    {
        vcr.seek_start_sample = vcr.current_sample;
        goto finish;
    }

    if (vcr.task == task_playback)
    {
        // Fast path: use seek savestates
        // FIXME: Duplicated code, a bit ugly
        if (g_core->cfg->seek_savestate_interval != 0)
        {
            g_core->log_trace("[VCR] vcr_begin_seek_impl: playback, fast path");

            // FIXME: Might be better to have read-only as an individual flag for each savestate, cause as it is now,
            // we're overwriting global state for  this...
            g_core->cfg->vcr_readonly = true;

            const auto closest_key = vcr_find_closest_savestate_before_frame(frame);

            vcr.seek_start_sample = closest_key;

            g_core->log_info(std::format(
                "[VCR] Seeking during playback to frame {}, loading closest savestate at {}...", frame, closest_key));
            vcr.seek_savestate_loading = true;

            // NOTE: This needs to go through AsyncExecutor (despite us already being on a worker thread) or it will
            // cause a deadlock.
            g_core->submit_task([=] {
                g_ctx.st_do_memory(
                    vcr.seek_savestates[closest_key], core_st_job_load,
                    [=](const core_st_callback_info &info, auto &&...) {
                        if (info.result != Res_Ok)
                        {
                            g_core->show_dialog("Failed to load seek savestate for seek operation.", "VCR", fsvc_error);
                            vcr.seek_savestate_loading = false;

                            {
                                vcr_anti_lock bypass;
                                g_ctx.vcr_stop_seek();
                            }
                        }

                        g_core->log_info(std::format("[VCR] Seek savestate at frame {} loaded!", closest_key));
                        vcr.seek_savestate_loading = false;
                    },
                    false);
            });

            goto finish;
        }

        vcr.seek_start_sample = 0;

        g_core->log_trace("[VCR] vcr_begin_seek_impl: playback, slow path");

        core_result result;
        {
            vcr_anti_lock bypass;
            result = g_ctx.vcr_start_playback(vcr.movie_path);
        }

        if (result != Res_Ok)
        {
            g_core->log_error(
                std::format("[VCR] vcr_begin_seek_impl: core_vcr_start_playback failed with error code {}",
                            static_cast<int32_t>(result)));
            vcr.seek_to_frame.reset();

            {
                vcr_anti_lock bypass;
                g_core->callbacks.seek_status_changed();
            }

            return result;
        }

        goto finish;
    }

    if (vcr.task == task_recording)
    {
        if (g_core->cfg->seek_savestate_interval == 0)
        {
            // TODO: We can't backtrack using savestates, so we'd have to restart into recording mode while restoring
            // the buffer, leave it for the next release...
            g_core->show_dialog("The seek savestate interval can't be 0 when seeking backwards during recording.",
                                "VCR", fsvc_error);
            return VCR_SeekSavestateIntervalZero;
        }

        const auto target_sample = warp_modify ? vcr.warp_modify_first_difference_frame : frame;

        // All seek savestates after the target frame need to be purged, as the user will invalidate them by overwriting
        // inputs prior to them
        if (!g_core->cfg->vcr_readonly)
        {
            std::vector<size_t> to_erase;
            for (const auto &[sample, _] : vcr.seek_savestates)
            {
                if (sample >= target_sample)
                {
                    to_erase.push_back(sample);
                }
            }
            for (const auto sample : to_erase)
            {
                g_core->log_info(std::format("[VCR] Erasing now-invalidated seek savestate at frame {}...", sample));
                vcr.seek_savestates.erase(sample);
                post_unlock_callbacks.push([=] { g_core->callbacks.seek_savestate_changed(sample); });
            }
        }

        const auto closest_key = vcr_find_closest_savestate_before_frame(target_sample);

        vcr.seek_start_sample = closest_key;

        g_core->log_info(
            std::format("[VCR] Seeking backwards during recording to frame {}, loading closest savestate at {}...",
                        target_sample, closest_key));
        vcr.seek_savestate_loading = true;

        // NOTE: This needs to go through AsyncExecutor (despite us already being on a worker thread) or it will cause a
        // deadlock.
        g_core->submit_task([=] {
            g_ctx.st_do_memory(
                vcr.seek_savestates[closest_key], core_st_job_load,
                [=](const core_st_callback_info &info, auto &&...) {
                    if (info.result != Res_Ok)
                    {
                        g_core->show_dialog("Failed to load seek savestate for seek operation.", "VCR", fsvc_error);
                        vcr.seek_savestate_loading = false;

                        {
                            vcr_anti_lock bypass;
                            g_ctx.vcr_stop_seek();
                        }
                    }

                    g_core->log_info(std::format("[VCR] Seek savestate at frame {} loaded!", closest_key));
                    vcr.seek_savestate_loading = false;
                },
                false);
        });
    }

finish: {
    vcr_anti_lock bypass;
    while (!post_unlock_callbacks.empty())
    {
        post_unlock_callbacks.front()();
        post_unlock_callbacks.pop();
    }

    g_core->callbacks.readonly_changed((bool)g_core->cfg->vcr_readonly);
    g_core->callbacks.seek_status_changed();
}
    return Res_Ok;
}

core_result vcr_begin_seek(std::string str, bool pause_at_end)
{
    return vcr_begin_seek_impl(str, pause_at_end, true, false);
}

void vcr_stop_seek()
{
    // We need to acquire the mutex here, as this function is also called during input poll
    // and having two of these running at the same time is bad for obvious reasons
    std::unique_lock lock(vcr_mtx);

    if (!vcr.seek_to_frame.has_value())
    {
        g_core->log_info("[VCR] Tried to call stop_seek with no seek operation running");
        return;
    }

    vcr.seek_to_frame.reset();

    if (vcr.warp_modify_active)
    {
        vcr.warp_modify_active = false;
    }

    {
        vcr_anti_lock bypass;
        g_core->callbacks.seek_status_changed();
        g_core->callbacks.seek_completed();
        g_core->callbacks.warp_modify_status_changed(vcr.warp_modify_active);
    }
}

bool vcr_is_seeking()
{
    std::unique_lock lock(vcr_mtx);
    return vcr.seek_to_frame.has_value();
}

bool vcr_is_task_recording(core_vcr_task task)
{
    return task == task_recording || task == task_start_recording_from_reset ||
           task == task_start_recording_from_snapshot;
}

static void vcr_clear_seek_savestates(std::queue<std::function<void()>> &post_unlock_callbacks)
{
    g_core->log_info("[VCR] Clearing seek savestates...");

    std::vector<size_t> prev_seek_savestate_keys;
    prev_seek_savestate_keys.reserve(vcr.seek_savestates.size());
    for (const auto &[key, _] : vcr.seek_savestates)
    {
        prev_seek_savestate_keys.push_back(key);
    }

    vcr.seek_savestates.clear();

    for (const auto frame : prev_seek_savestate_keys)
    {
        post_unlock_callbacks.emplace([=] { g_core->callbacks.seek_savestate_changed(frame); });
    }
}

core_result vcr_stop_all()
{
    std::unique_lock lock(vcr_mtx);
    std::queue<std::function<void()>> post_unlock_callbacks{};

    const bool is_recording = vcr.task == task_start_recording_from_reset ||
                              vcr.task == task_start_recording_from_snapshot || vcr.task == task_recording;
    const bool is_playback = vcr.task == task_start_playback_from_reset ||
                             vcr.task == task_start_playback_from_snapshot || vcr.task == task_playback;

    if (!is_recording && !is_playback)
    {
        return Res_Ok;
    }

    vcr_clear_seek_savestates(post_unlock_callbacks);

    if (is_recording || is_playback)
    {
        for (int i = 0; i < 4; i++)
        {
            g_core->input_set_keys(i, {.value = 0});
        }
    }

    if (is_recording)
    {
        if (vcr.task == task_start_recording_from_reset)
        {
            vcr.task = task_idle;
            g_core->log_info("[VCR] Removing files (nothing recorded)");

            auto current_path = std::filesystem::path(vcr.movie_path);

            current_path.replace_extension(MUPEN64_PATH_T(".m64"));
            std::filesystem::remove(current_path);

            current_path.replace_extension(MUPEN64_PATH_T(".st"));
            std::filesystem::remove(current_path);
        }

        if (vcr.task == task_recording)
        {
            write_movie();

            vcr.task = task_idle;

            g_core->log_info(
                std::format("[VCR] Recording stopped. Recorded %ld input samples", vcr.hdr.length_samples));
        }

        {
            vcr_anti_lock bypass;
            execute_post_unlock_callbacks(post_unlock_callbacks);
            g_core->callbacks.task_changed(vcr.task);
        }

        return Res_Ok;
    }

    if (is_playback)
    {
        vcr.task = task_idle;
        cht_layer_pop();

        {
            vcr_anti_lock bypass;
            execute_post_unlock_callbacks(post_unlock_callbacks);
            g_core->callbacks.task_changed(vcr.task);
            g_core->callbacks.stop_movie();
        }

        return Res_Ok;
    }

    return Res_Ok;
}

std::filesystem::path vcr_get_path()
{
    std::unique_lock lock(vcr_mtx);
    return vcr.movie_path;
}

core_vcr_task vcr_get_task()
{
    return vcr.task;
}

uint32_t vcr_get_length_samples()
{
    std::unique_lock lock(vcr_mtx);
    return vcr.task == task_idle ? UINT32_MAX : vcr.hdr.length_samples;
}

uint32_t vcr_get_length_vis()
{
    std::unique_lock lock(vcr_mtx);
    return vcr.task == task_idle ? UINT32_MAX : vcr.hdr.length_vis;
}

int32_t vcr_get_current_vi()
{
    std::unique_lock lock(vcr_mtx);
    return vcr.task == task_idle ? -1 : vcr.current_vi;
}

std::vector<core_buttons> vcr_get_inputs()
{
    std::unique_lock lock(vcr_mtx);
    return vcr.inputs;
}

/// Finds the first input difference between two input vectors. Returns SIZE_MAX if they are identical.
size_t vcr_find_first_input_difference(const std::vector<core_buttons> &first, const std::vector<core_buttons> &second)
{
    if (first.size() != second.size())
    {
        const auto min_size = std::min(first.size(), second.size());
        for (int32_t i = 0; i < min_size; ++i)
        {
            if (first[i].value != second[i].value)
            {
                return i;
            }
        }
        return std::max(0, (int32_t)min_size - 1);
    }

    for (int32_t i = 0; i < first.size(); ++i)
    {
        if (first[i].value != second[i].value)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

core_result vcr_begin_warp_modify(const std::vector<core_buttons> &inputs)
{
    std::unique_lock lock(vcr_mtx);

    if (vcr.warp_modify_active)
    {
        return VCR_WarpModifyAlreadyRunning;
    }

    if (vcr.task != task_recording)
    {
        return VCR_WarpModifyNeedsRecordingTask;
    }

    if (inputs.empty())
    {
        return VCR_WarpModifyEmptyInputBuffer;
    }

    vcr.warp_modify_first_difference_frame = vcr_find_first_input_difference(vcr.inputs, inputs);

    if (vcr.warp_modify_first_difference_frame == SIZE_MAX)
    {
        g_core->log_info("[VCR] Warp modify inputs are identical to current input buffer, doing nothing...");

        vcr.warp_modify_active = false;

        {
            vcr_anti_lock bypass;
            g_core->callbacks.warp_modify_status_changed(vcr.warp_modify_active);
        }

        return Res_Ok;
    }

    if (vcr.warp_modify_first_difference_frame > vcr.current_sample)
    {
        g_core->log_info(std::format("[VCR] First different frame is in the future (current sample: {}, first "
                                     "differenece: {}), copying inputs with no seek...",
                                     vcr.current_sample, vcr.warp_modify_first_difference_frame));

        vcr.inputs = inputs;
        vcr.hdr.length_samples = vcr.inputs.size();

        vcr.warp_modify_active = false;
        vcr_increment_rerecord_count();

        {
            vcr_anti_lock bypass;
            g_core->callbacks.warp_modify_status_changed(vcr.warp_modify_active);
            g_core->callbacks.rerecords_changed(get_rerecord_count());
        }

        return Res_Ok;
    }

    const auto target_sample = std::min(inputs.size(), (size_t)vcr.current_sample);

    const auto result =
        vcr_begin_seek_impl(std::to_string(target_sample), emu_paused || frame_advance_outstanding, false, true);

    if (result != Res_Ok)
    {
        return result;
    }

    g_core->log_info(std::format("[VCR] Warp modify started at frame {}", vcr.current_sample));

    vcr_increment_rerecord_count();

    vcr.inputs = inputs;
    vcr.hdr.length_samples = vcr.inputs.size();
    vcr.warp_modify_active = true;
    g_ctx.vr_resume_emu();

    {
        vcr_anti_lock bypass;
        g_core->callbacks.warp_modify_status_changed(vcr.warp_modify_active);
        g_core->callbacks.rerecords_changed(get_rerecord_count());
    }

    return Res_Ok;
}

bool vcr_get_warp_modify_status()
{
    std::unique_lock lock(vcr_mtx);

    return vcr.warp_modify_active;
}

size_t vcr_get_warp_modify_first_difference_frame()
{
    std::unique_lock lock(vcr_mtx);

    return vcr_get_warp_modify_status() == vcr.warp_modify_active ? vcr.warp_modify_first_difference_frame : SIZE_MAX;
}

void vcr_get_seek_savestate_frames(std::unordered_map<size_t, bool> &map)
{
    std::unique_lock lock(vcr_mtx);

    map.clear();

    for (const auto &[key, _] : vcr.seek_savestates)
    {
        map[key] = true;
    }
}

bool vcr_has_seek_savestate_at_frame(const size_t frame)
{
    std::unique_lock lock(vcr_mtx);
    return vcr.seek_savestates.contains(frame);
}

void vcr_on_vi()
{
    std::unique_lock lock(vcr_mtx);

    vcr.current_vi++;

    if (vcr.task == task_recording && !vcr.warp_modify_active) vcr.hdr.length_vis = vcr.current_vi;

    if (vcr.task != task_playback) return;

    bool pausing_at_last = (g_core->cfg->pause_at_last_frame && vcr.current_sample == vcr.hdr.length_samples);
    bool pausing_at_n = (g_core->cfg->pause_at_frame != -1 && vcr.current_sample >= g_core->cfg->pause_at_frame);

    if (pausing_at_last || pausing_at_n)
    {
        vcr_anti_lock bypass;
        g_ctx.vr_pause_emu();
    }

    if (pausing_at_last)
    {
        g_core->cfg->pause_at_last_frame = 0;
    }

    if (pausing_at_n)
    {
        g_core->cfg->pause_at_frame = -1;
    }
}

bool vcr_is_frame_skipped()
{
    std::unique_lock lock(vcr_mtx);

    if (frame_advance_outstanding > 1)
    {
        return true;
    }

    if (!g_core->cfg->render_throttling)
    {
        return false;
    }

    if (vcr.seek_to_frame.has_value())
    {
        return true;
    }

    if (!g_vr_fast_forward)
    {
        return false;
    }

    // skip every frame
    if (g_core->cfg->frame_skip_frequency == 0)
    {
        return true;
    }

    // skip no frames
    if (g_core->cfg->frame_skip_frequency == 1)
    {
        return false;
    }

    return g_total_frames % g_core->cfg->frame_skip_frequency != 0;
}
