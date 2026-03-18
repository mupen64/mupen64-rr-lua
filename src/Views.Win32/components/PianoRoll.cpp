/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "PianoRoll.h"
#include "ThreadPool.h"
#include "Config.h"
#include "Messenger.h"
#include <components/CoreUtils.h>

const auto JOYSTICK_CLASS = L"PianoRollJoystick";

struct piano_roll_history_state
{
    // The input buffer for the piano roll, which is a copy of the inputs from the core and is modified by the user.
    // When editing operations end, this buffer is provided to begin_warp_modify and thereby applied to the core,
    // changing the resulting emulator state.
    std::vector<core_buttons> inputs;

    // Selected indicies in the piano roll listview.
    std::vector<size_t> selected_indicies;
};

struct piano_roll_state
{
    HWND hwnd{};
    HWND lv_hwnd{};
    HWND joy_hwnd{};
    HWND hist_hwnd{};
    HWND status_hwnd{};

    // The clipboard buffer for piano roll copy/paste operations. Must be sorted ascendingly.
    //
    // Due to allowing sparse ("extended") selections, we might have gaps in the clipboard buffer as well as the
    // selected indicies when pasting. When a gap-containing clipboard buffer is pasted into the piano roll, the
    // selection acts as a mask.
    //
    // A----
    // -----
    // @@@@@ << Gap!
    // -B---
    // A----
    //
    // results in...
    //
    // $$$$$ << Unaffected, outside of selection!
    // ----- [[[ Selection start
    // $$$$$ << Unaffected
    // -B---
    // $$$$$ <<< Unaffected ]]] Selection end
    //
    // This also applies for the inverse (gapless clipboard buffer and g_piano_roll_state.selected_indicies with gaps).
    //
    std::vector<std::optional<core_buttons>> clipboard{};

    // Whether the current copy of the VCR inputs is desynced from the remote one.
    bool inputs_different{};

    // Whether a drag operation is happening
    bool lv_dragging{};

    // The value of the cell under the mouse at the time when the drag operation started
    bool lv_drag_initial_value{};

    // The column index of the drag operation.
    size_t lv_drag_column{};

    // Whether the drag operation should unset the affected buttons regardless of the initial value
    bool lv_drag_unset{};

    // HACK: Flag used to not show context menu when dragging in the button columns
    bool lv_ignore_context_menu{};

    // Whether a joystick drag operation is happening
    bool joy_drag{};

    // The current piano roll state.
    piano_roll_history_state current_state{};

    // State history for the piano roll. Used by undo/redo.
    std::deque<piano_roll_history_state> piano_roll_history{};

    // Stack index for the piano roll undo/redo stack. 0 = top, 1 = 2nd from top, etc...
    size_t state_index{};

    // Copy of seek savestate frame map from VCR.
    std::unordered_map<size_t, bool> seek_savestate_frames{};

    std::vector<std::function<void()>> unsubscribe_funcs{};

    bool readwrite = true;

    size_t current_sample{};
    size_t previous_sample{};
};

static piano_roll_state piano_roll{};

static void on_can_modify_inputs_changed()
{
}

static void update_can_modify_inputs()
{
    ThreadPool::submit_task([] {
        const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

        const auto prev_can_modify_inputs = piano_roll.readwrite;
        piano_roll.readwrite = !g_main_ctx.core_ctx->vcr_get_warp_modify_status() &&
                               info.seek_target_sample == SIZE_MAX &&
                               g_main_ctx.core_ctx->vcr_get_task() == task_recording &&
                               !g_main_ctx.core_ctx->vcr_is_seeking() && !g_config.core.vcr_readonly &&
                               g_config.core.seek_savestate_interval > 0 && g_main_ctx.core_ctx->vr_get_paused();

        if (prev_can_modify_inputs != piano_roll.readwrite)
        {
            g_main_ctx.dispatcher->invoke([] { on_can_modify_inputs_changed(); });
        }
    });
}

/**
 * Gets whether a seek operation can be initiated.
 */
static bool can_seek()
{
    return g_config.core.seek_savestate_interval > 0;
}

/**
 * Refreshes the piano roll listview and the joystick, re-querying the current inputs from the core.
 */
static void update_inputs()
{
    if (!piano_roll.hwnd)
    {
        return;
    }

    // If VCR is idle, we can't really show anything.
    if (g_main_ctx.core_ctx->vcr_get_task() == task_idle)
    {
        ListView_DeleteAllItems(piano_roll.lv_hwnd);
    }

    // In playback mode, the input buffer can't change so we're safe to only pull it once.
    if (g_main_ctx.core_ctx->vcr_get_task() == task_playback)
    {
        SetWindowRedraw(piano_roll.lv_hwnd, false);

        ListView_DeleteAllItems(piano_roll.lv_hwnd);

        piano_roll.current_state.inputs = g_main_ctx.core_ctx->vcr_get_inputs();
        ListView_SetItemCount(piano_roll.lv_hwnd, piano_roll.current_state.inputs.size());
        g_view_logger->info("[PianoRoll] Pulled inputs from core for playback mode, count: {}",
                            piano_roll.current_state.inputs.size());

        SetWindowRedraw(piano_roll.lv_hwnd, true);
    }

    RedrawWindow(piano_roll.joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

/**
 * Gets a button value from a BUTTONS struct at a given column index.
 * \param btn The BUTTONS struct to get the value from
 * \param i The column index. Must be in the range [3, 15] inclusive.
 * \return The button value at the given column index
 */
static unsigned get_input_value_from_column_index(core_buttons btn, size_t i)
{
    switch (i)
    {
    case 4:
        return btn.a;
    case 5:
        return btn.b;
    case 6:
        return btn.z;
    case 7:
        return btn.r;
    case 8:
        return btn.start;
    case 9:
        return btn.cu;
    case 10:
        return btn.cl;
    case 11:
        return btn.cr;
    case 12:
        return btn.cd;
    case 13:
        return btn.du;
    case 14:
        return btn.dl;
    case 15:
        return btn.dr;
    case 16:
        return btn.dd;
    default:
        assert(false);
        return -1;
    }
}

/**
 * Sets a button value in a BUTTONS struct at a given column index.
 * \param btn The BUTTONS struct to set the value in
 * \param i The column index. Must be in the range [3, 15] inclusive.
 * \param value The button value to set
 */
static void set_input_value_from_column_index(core_buttons *btn, size_t i, bool value)
{
    switch (i)
    {
    case 4:
        btn->a = value;
        break;
    case 5:
        btn->b = value;
        break;
    case 6:
        btn->z = value;
        break;
    case 7:
        btn->r = value;
        break;
    case 8:
        btn->start = value;
        break;
    case 9:
        btn->cu = value;
        break;
    case 10:
        btn->cl = value;
        break;
    case 11:
        btn->cr = value;
        break;
    case 12:
        btn->cd = value;
        break;
    case 13:
        btn->du = value;
        break;
    case 14:
        btn->dl = value;
        break;
    case 15:
        btn->dr = value;
        break;
    case 16:
        btn->dd = value;
        break;
    default:
        assert(false);
        break;
    }
}

/**
 * Gets the button name from a column index.
 * \param i The column index. Must be in the range [3, 15] inclusive.
 * \return The name of the button at the specified column index.
 */
static const wchar_t *get_button_name_from_column_index(size_t i)
{
    switch (i)
    {
    case 4:
        return L"A";
    case 5:
        return L"B";
    case 6:
        return L"Z";
    case 7:
        return L"R";
    case 8:
        return L"S";
    case 9:
        return L"C^";
    case 10:
        return L"C<";
    case 11:
        return L"C>";
    case 12:
        return L"Cv";
    case 13:
        return L"D^";
    case 14:
        return L"D<";
    case 15:
        return L"D>";
    case 16:
        return L"Dv";
    default:
        assert(false);
        return nullptr;
    }
}

/**
 * Prints a dump of the clipboard
 */
static void print_clipboard_dump()
{
    g_view_logger->info("[PianoRoll] ------------- Dump Begin -------------");
    g_view_logger->info("[PianoRoll] Clipboard of length {}", piano_roll.clipboard.size());
    for (auto item : piano_roll.clipboard)
    {
        item.has_value() ? g_view_logger->info("[PianoRoll] {:#06x}", item.value().value)
                         : g_view_logger->info("[PianoRoll] ------");
    }
    g_view_logger->info("[PianoRoll] ------------- Dump End -------------");
}

/**
 * Ensures that the currently relevant item is visible in the piano roll listview.
 */
static void ensure_relevant_item_visible()
{
    const int32_t i = ListView_GetNextItem(piano_roll.lv_hwnd, -1, LVNI_SELECTED);
    const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

    const auto current_sample =
        std::min(ListView_GetItemCount(piano_roll.lv_hwnd), static_cast<int32_t>(info.current_sample) + 10);
    const auto playhead_sample =
        g_main_ctx.core_ctx->vcr_get_task() == task_recording ? current_sample - 1 : current_sample;

    if (g_config.piano_roll_keep_playhead_visible)
    {
        ListView_EnsureVisible(piano_roll.lv_hwnd, playhead_sample, false);
    }

    if (g_config.piano_roll_keep_selection_visible && i != -1)
    {
        ListView_EnsureVisible(piano_roll.lv_hwnd, i, false);
    }
}

/**
 * Copies the selected inputs to the clipboard.
 */
static void copy_inputs()
{
    if (piano_roll.current_state.selected_indicies.empty())
    {
        return;
    }

    if (piano_roll.current_state.selected_indicies.size() == 1)
    {
        piano_roll.clipboard = {piano_roll.current_state.inputs[piano_roll.current_state.selected_indicies[0]]};
        return;
    }

    const size_t min = piano_roll.current_state.selected_indicies[0];
    const size_t max =
        piano_roll.current_state.selected_indicies[piano_roll.current_state.selected_indicies.size() - 1];

    piano_roll.clipboard.clear();
    piano_roll.clipboard.reserve(max - min);

    for (auto i = min; i <= max; ++i)
    {
        // FIXME: Precompute this, create a map, do anything but not this bru
        const bool gap = std::ranges::find(piano_roll.current_state.selected_indicies, i) ==
                         piano_roll.current_state.selected_indicies.end();
        // HACK: nullopt acquired via explicit constructor call...
        std::optional<core_buttons> opt;
        piano_roll.clipboard.push_back(gap ? opt : piano_roll.current_state.inputs[i]);
    }

    print_clipboard_dump();
}

/**
 * Updates the history listbox with the current state of the undo/redo stack.
 */
static void update_history_listbox()
{
    SetWindowRedraw(piano_roll.hist_hwnd, false);
    ListBox_ResetContent(piano_roll.hist_hwnd);

    for (size_t i = 0; i < piano_roll.piano_roll_history.size(); ++i)
    {
        ListBox_AddString(piano_roll.hist_hwnd, std::format(L"Snapshot {}", i + 1).c_str());
    }

    ListBox_SetCurSel(piano_roll.hist_hwnd, piano_roll.state_index);

    SetWindowRedraw(piano_roll.hist_hwnd, true);
}

/**
 * Pushes the current piano roll state to the history. Should be called after operations which change the piano roll
 * state.
 */
static void push_state_to_history()
{
    g_view_logger->info("[PianoRoll] Pushing state to undo stack...");

    if (piano_roll.piano_roll_history.size() > g_config.piano_roll_undo_stack_size)
    {
        piano_roll.piano_roll_history.pop_back();
    }

    piano_roll.piano_roll_history.push_back(piano_roll.current_state);
    piano_roll.state_index = std::min(piano_roll.state_index + 1, piano_roll.piano_roll_history.size() - 1);

    g_view_logger->info("[PianoRoll] Undo stack size: {}. Current index: {}.", piano_roll.piano_roll_history.size(),
                        piano_roll.state_index);
    update_history_listbox();
}

/**
 * Applies the g_piano_roll_state.inputs buffer to the core.
 */
static void apply_input_buffer(bool push_to_history = true)
{
    if (!piano_roll.inputs_different)
    {
        g_view_logger->trace("[PianoRoll] Ignoring apply_input_buffer because inputs didn't change.");
        return;
    }

    if (!piano_roll.readwrite)
    {
        g_view_logger->warn("[PianoRoll] Tried to call apply_input_buffer, but g_can_modify_inputs == false.");
        return;
    }

    // This might be called from UI thread, thus grabbing the VCR lock.
    // Problem is that the VCR lock is already grabbed by the core thread because current sample changed message is
    // executed on core thread.
    ThreadPool::submit_task([=] {
        auto result = g_main_ctx.core_ctx->vcr_begin_warp_modify(piano_roll.current_state.inputs);
        const auto inputs = g_main_ctx.core_ctx->vcr_get_inputs();

        g_main_ctx.dispatcher->invoke([=] {
            if (result == Res_Ok)
            {
                if (push_to_history)
                {
                    push_state_to_history();
                }
            }
            else
            {
                // Since we do optimistic updates, we need to revert the changes we made to the input buffer to avoid
                // visual desync
                SetWindowRedraw(piano_roll.lv_hwnd, false);

                ListView_DeleteAllItems(piano_roll.lv_hwnd);

                piano_roll.current_state.inputs = inputs;
                ListView_SetItemCount(piano_roll.lv_hwnd, piano_roll.current_state.inputs.size());
                g_view_logger->info(
                    "[PianoRoll] Pulled inputs from core for recording mode due to warp modify failing, count: {}",
                    piano_roll.current_state.inputs.size());

                SetWindowRedraw(piano_roll.lv_hwnd, true);

                CoreUtils::show_error_dialog_for_result(result, piano_roll.hwnd);
            }

            piano_roll.inputs_different = false;
        });
    });
}

/**
 * Sets the piano roll state to the specified value, updating everything accordingly and also applying the input buffer.
 * This is an expensive and slow operation.
 */
static void set_piano_roll_state(const piano_roll_history_state state)
{
    piano_roll.current_state = state;

    piano_roll.inputs_different = true;
    ListView_SetItemCountEx(piano_roll.lv_hwnd, piano_roll.current_state.inputs.size(), LVSICF_NOSCROLL);
    set_listview_selection(piano_roll.lv_hwnd, piano_roll.current_state.selected_indicies);
    apply_input_buffer(false);
    update_history_listbox();
}

/**
 * Shifts the history index by the specified offset and applies the changes.
 */
static bool shift_history(int offset)
{
    auto new_index = piano_roll.state_index + offset;

    if (new_index < 0 || new_index >= piano_roll.piano_roll_history.size())
    {
        return false;
    }

    piano_roll.state_index = new_index;
    set_piano_roll_state(piano_roll.piano_roll_history[piano_roll.state_index]);

    return true;
}

/**
 * Restores the piano roll state to the last stored state.
 */
static bool undo()
{
    return shift_history(-1);
}

/**
 * Restores the piano roll state to the next stored state.
 */
static bool redo()
{
    return shift_history(1);
}

/**
 * Pastes the selected inputs from the clipboard into the piano roll.
 */
static void paste_inputs(bool merge)
{
    if (piano_roll.clipboard.empty() || piano_roll.current_state.selected_indicies.empty() || !piano_roll.readwrite)
    {
        return;
    }

    bool clipboard_has_gaps = false;
    for (auto item : piano_roll.clipboard)
    {
        if (!item.has_value())
        {
            clipboard_has_gaps = true;
            break;
        }
    }

    bool selection_has_gaps = false;
    if (piano_roll.current_state.selected_indicies.size() > 1)
    {
        for (int i = 1; i < piano_roll.current_state.selected_indicies.size(); ++i)
        {
            if (piano_roll.current_state.selected_indicies[i] - piano_roll.current_state.selected_indicies[i - 1] > 1)
            {
                selection_has_gaps = true;
                break;
            }
        }
    }

    g_view_logger->info("[PianoRoll] Clipboard/selection gaps: {}, {}", clipboard_has_gaps, selection_has_gaps);

    SetWindowRedraw(piano_roll.lv_hwnd, false);

    if (piano_roll.current_state.selected_indicies.size() == 1)
    {
        // 1-sized selection indicates a bulk copy, where copy all the inputs over (and ignore the clipboard gaps)
        size_t i = piano_roll.current_state.selected_indicies[0];

        for (auto item : piano_roll.clipboard)
        {
            if (item.has_value() && i < piano_roll.current_state.inputs.size())
            {
                piano_roll.current_state.inputs[i] =
                    merge ? core_buttons{piano_roll.current_state.inputs[i].value | item.value().value} : item.value();
                ListView_Update(piano_roll.lv_hwnd, i);
            }

            i++;
        }
    }
    else
    {
        // Standard case: selection is a mask
        size_t i = piano_roll.current_state.selected_indicies[0];

        for (auto item : piano_roll.clipboard)
        {
            const bool included = std::ranges::find(piano_roll.current_state.selected_indicies, i) !=
                                  piano_roll.current_state.selected_indicies.end();

            if (item.has_value() && i < piano_roll.current_state.inputs.size() && included)
            {
                piano_roll.current_state.inputs[i] =
                    merge ? core_buttons{piano_roll.current_state.inputs[i].value | item.value().value} : item.value();
                ListView_Update(piano_roll.lv_hwnd, i);
            }

            i++;
        }
    }

    // Move selection to allow cool block-wise pasting
    const size_t offset =
        piano_roll.current_state.selected_indicies[piano_roll.current_state.selected_indicies.size() - 1] -
        piano_roll.current_state.selected_indicies[0] + 1;
    shift_listview_selection(piano_roll.lv_hwnd, offset);

    ensure_relevant_item_visible();

    SetWindowRedraw(piano_roll.lv_hwnd, true);

    piano_roll.inputs_different = true;
    apply_input_buffer();
}

/**
 * Zeroes out all inputs in the current selection
 */
static void clear_inputs_in_selection()
{
    if (piano_roll.current_state.selected_indicies.empty() || !piano_roll.readwrite)
    {
        return;
    }

    SetWindowRedraw(piano_roll.lv_hwnd, false);

    for (auto i : piano_roll.current_state.selected_indicies)
    {
        piano_roll.current_state.inputs[i] = {0};
        ListView_Update(piano_roll.lv_hwnd, i);
    }

    SetWindowRedraw(piano_roll.lv_hwnd, true);

    piano_roll.inputs_different = true;
    apply_input_buffer();
}

/**
 * Deletes all inputs in the current selection, removing them from the input buffer.
 */
static void delete_inputs_in_selection()
{
    if (piano_roll.current_state.selected_indicies.empty() || !piano_roll.readwrite)
    {
        return;
    }

    std::vector selected_indicies(piano_roll.current_state.selected_indicies.begin(),
                                  piano_roll.current_state.selected_indicies.end());
    piano_roll.current_state.inputs = MiscHelpers::erase_indices(piano_roll.current_state.inputs, selected_indicies);
    ListView_RedrawItems(piano_roll.lv_hwnd, 0, ListView_GetItemCount(piano_roll.lv_hwnd));
    const int32_t offset =
        piano_roll.current_state.selected_indicies[piano_roll.current_state.selected_indicies.size() - 1] -
        piano_roll.current_state.selected_indicies[0] + 1;
    shift_listview_selection(piano_roll.lv_hwnd, -offset);

    piano_roll.inputs_different = true;
    apply_input_buffer();
}

/**
 * Appends the specified amount of empty frames at the start of the current selection.
 */
static bool insert_frames(size_t count)
{
    if (!piano_roll.readwrite || piano_roll.current_state.selected_indicies.empty())
    {
        return false;
    }

    for (int i = 0; i < count; ++i)
    {
        piano_roll.current_state.inputs.insert(
            piano_roll.current_state.inputs.begin() + piano_roll.current_state.selected_indicies[0] + 1, {0});
    }

    ListView_SetItemCountEx(piano_roll.lv_hwnd, piano_roll.current_state.inputs.size(), LVSICF_NOSCROLL);

    piano_roll.inputs_different = true;
    apply_input_buffer();

    return true;
}

static void update_groupbox_status_text()
{
    ThreadPool::submit_task([] {
        const auto warp_modify_active = g_main_ctx.core_ctx->vcr_get_warp_modify_status();
        const auto paused = g_main_ctx.core_ctx->vr_get_paused();

        g_main_ctx.dispatcher->invoke([=] {
            if (warp_modify_active)
            {
                SetWindowText(piano_roll.hwnd, L"Piano Roll - Warping...");
                return;
            }

            if (!paused)
            {
                SetWindowText(piano_roll.hwnd, L"Piano Roll - Resumed (readonly)");
                return;
            }

            if (piano_roll.current_state.selected_indicies.empty())
            {
                SetWindowText(piano_roll.hwnd, L"Piano Roll");
            }
            else if (piano_roll.current_state.selected_indicies.size() == 1)
            {
                SetWindowText(
                    piano_roll.hwnd,
                    std::format(L"Piano Roll - Frame {}", piano_roll.current_state.selected_indicies[0]).c_str());
            }
            else
            {
                SetWindowText(piano_roll.hwnd, std::format(L"Piano Roll - {} frames selected",
                                                           piano_roll.current_state.selected_indicies.size())
                                                   .c_str());
            }
        });
    });
}

/**
 * Gets whether the joystick control can be interacted with by the user.
 */
static bool can_joystick_be_modified()
{
    return !piano_roll.current_state.selected_indicies.empty() && piano_roll.readwrite;
}

static void on_task_changed(std::any data)
{
    update_can_modify_inputs();

    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<core_vcr_task>(data);
        static auto previous_value = value;

        if (value != previous_value)
        {
            g_view_logger->info("[PianoRoll] Processing TaskChanged from {} to {}", (int32_t)previous_value,
                                (int32_t)value);
            update_inputs();
        }

        if (g_config.core.seek_savestate_interval == 0)
        {
            SetWindowText(piano_roll.status_hwnd, L"Piano Roll read-only.\nSeek savestates must be enabled.");
        }
        else
        {
            SetWindowText(piano_roll.status_hwnd, L"");
        }

        previous_value = value;
    });
}

static void on_current_sample_changed(std::any data)
{
    piano_roll.previous_sample = piano_roll.current_sample;
    piano_roll.current_sample = g_main_ctx.core_ctx->vcr_get_seek_info().current_sample;

    if (g_main_ctx.core_ctx->vcr_get_warp_modify_status() || g_main_ctx.core_ctx->vcr_is_seeking())
    {
        return;
    }

    if (g_main_ctx.core_ctx->vcr_get_task() == task_idle)
    {
        return;
    }

    g_main_ctx.dispatcher->invoke([=] {
        if (g_main_ctx.core_ctx->vcr_get_task() == task_recording)
        {
            piano_roll.current_state.inputs = g_main_ctx.core_ctx->vcr_get_inputs();
            ListView_SetItemCountEx(piano_roll.lv_hwnd, piano_roll.current_state.inputs.size(), LVSICF_NOSCROLL);
        }

        ListView_Update(piano_roll.lv_hwnd, piano_roll.previous_sample);
        ListView_Update(piano_roll.lv_hwnd, piano_roll.current_sample);

        ensure_relevant_item_visible();
    });
}

static void on_unfreeze_completed(std::any)
{
    g_main_ctx.dispatcher->invoke([=] {
        if (g_main_ctx.core_ctx->vcr_get_warp_modify_status() || g_main_ctx.core_ctx->vcr_is_seeking())
        {
            return;
        }

        SetWindowRedraw(piano_roll.lv_hwnd, false);

        ListView_DeleteAllItems(piano_roll.lv_hwnd);

        piano_roll.current_state.inputs = g_main_ctx.core_ctx->vcr_get_inputs();
        const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

        const auto item_count = g_main_ctx.core_ctx->vcr_get_task() == task_recording
                                    ? std::min(info.current_sample, piano_roll.current_state.inputs.size())
                                    : piano_roll.current_state.inputs.size();

        g_view_logger->info("[PianoRoll] Setting item count to {} (input count: {})...", item_count,
                            piano_roll.current_state.inputs.size());
        ListView_SetItemCountEx(piano_roll.lv_hwnd, item_count, LVSICF_NOSCROLL);

        SetWindowRedraw(piano_roll.lv_hwnd, true);

        piano_roll.inputs_different = true;
        ensure_relevant_item_visible();
    });
}

static void on_warp_modify_status_changed(std::any)
{
    update_can_modify_inputs();

    g_main_ctx.dispatcher->invoke([=] {
        update_groupbox_status_text();
        RedrawWindow(piano_roll.joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    });
}

static void on_seek_completed(std::any)
{
    update_can_modify_inputs();

    g_main_ctx.dispatcher->invoke([=] { RedrawWindow(piano_roll.joy_hwnd, nullptr, nullptr, RDW_INVALIDATE); });
}

static void on_seek_savestate_changed(std::any data)
{
    g_main_ctx.dispatcher->invoke([=] {
        auto value = std::any_cast<size_t>(data);
        g_main_ctx.core_ctx->vcr_get_seek_savestate_frames(piano_roll.seek_savestate_frames);
        ListView_Update(piano_roll.lv_hwnd, value);
    });
}

static void on_emu_paused_changed(std::any)
{
    // Redrawing during frame advance (paused on, then off next frame) causes ugly flicker, so we'll just not do that
    if (g_main_ctx.core_ctx->vr_get_frame_advance() && !g_main_ctx.core_ctx->vr_get_paused())
    {
        return;
    }

    update_can_modify_inputs();

    g_main_ctx.dispatcher->invoke([=] {
        update_groupbox_status_text();
        RedrawWindow(piano_roll.joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    });
}

static void get_joystick_pens(HPEN *outline_pen, HPEN *line_pen, HPEN *tip_pen)
{
    if (can_joystick_be_modified())
    {
        *outline_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        *line_pen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
        *tip_pen = CreatePen(PS_SOLID, 7, RGB(255, 0, 0));
    }
    else
    {
        *outline_pen = CreatePen(PS_SOLID, 1, RGB(204, 204, 204));
        *line_pen = CreatePen(PS_SOLID, 3, RGB(229, 229, 229));
        *tip_pen = CreatePen(PS_SOLID, 7, RGB(235, 235, 235));
    }
}

/**
 * The window procedure for the joystick control.
 */
static LRESULT CALLBACK joystick_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
        if (!can_joystick_be_modified())
        {
            break;
        }
        piano_roll.joy_drag = true;
        SetCapture(hwnd);
        goto mouse_move;
    case WM_LBUTTONUP:
        if (!piano_roll.joy_drag)
        {
            break;
        }
        goto lmb_up;
    case WM_MOUSEMOVE:
        goto mouse_move;
    case WM_PAINT: {
        core_buttons input = {0};

        HPEN outline_pen;
        HPEN line_pen;
        HPEN tip_pen;
        get_joystick_pens(&outline_pen, &line_pen, &tip_pen);

        if (!piano_roll.current_state.selected_indicies.empty() &&
            piano_roll.current_state.selected_indicies[0] < piano_roll.current_state.inputs.size())
        {
            input = piano_roll.current_state.inputs[piano_roll.current_state.selected_indicies[0]];
        }

        g_view_logger->info("[PianoRoll] Joystick repaint, can_joystick_be_modified: {}", can_joystick_be_modified());

        PAINTSTRUCT ps;
        RECT rect;
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC cdc = CreateCompatibleDC(hdc);
        GetClientRect(hwnd, &rect);

        HBITMAP bmp = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        SelectObject(cdc, bmp);

        const int mid_x = rect.right / 2;
        const int mid_y = rect.bottom / 2;
        const int stick_x = (input.x + 128) * rect.right / 256;
        const int stick_y = (-input.y + 128) * rect.bottom / 256;

        FillRect(cdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

        SelectObject(cdc, outline_pen);
        Ellipse(cdc, 0, 0, rect.right, rect.bottom);

        MoveToEx(cdc, 0, mid_y, nullptr);
        LineTo(cdc, rect.right, mid_y);
        MoveToEx(cdc, mid_x, 0, nullptr);
        LineTo(cdc, mid_x, rect.bottom);

        SelectObject(cdc, line_pen);
        MoveToEx(cdc, mid_x, mid_y, nullptr);
        LineTo(cdc, stick_x, stick_y);

        SelectObject(cdc, tip_pen);
        MoveToEx(cdc, stick_x, stick_y, nullptr);
        LineTo(cdc, stick_x, stick_y);

        SelectObject(cdc, nullptr);

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, cdc, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);

        DeleteDC(cdc);
        DeleteObject(bmp);
        DeleteObject(outline_pen);
        DeleteObject(line_pen);
        DeleteObject(tip_pen);

        return 0;
    }
    default:
        break;
    }

def:
    return DefWindowProc(hwnd, msg, wParam, lParam);

lmb_up:
    ReleaseCapture();
    apply_input_buffer();
    piano_roll.joy_drag = false;
    goto def;

mouse_move:
    if (!piano_roll.readwrite)
    {
        piano_roll.joy_drag = false;
    }

    if (!piano_roll.joy_drag)
    {
        goto def;
    }

    // Apply the joystick input...
    if (!(GetKeyState(VK_LBUTTON) & 0x100))
    {
        goto lmb_up;
    }

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    RECT pic_rect;
    GetWindowRect(hwnd, &pic_rect);
    int x = (pt.x * UINT8_MAX / (signed)(pic_rect.right - pic_rect.left) - INT8_MAX + 1);
    int y = -(pt.y * UINT8_MAX / (signed)(pic_rect.bottom - pic_rect.top) - INT8_MAX + 1);

    if (x > INT8_MAX || y > INT8_MAX || x < INT8_MIN || y < INT8_MIN)
    {
        int div = std::max(abs(x), abs(y));
        x = x * INT8_MAX / div;
        y = y * INT8_MAX / div;
    }

    if (abs(x) <= 8) x = 0;
    if (abs(y) <= 8) y = 0;

    SetWindowRedraw(piano_roll.lv_hwnd, false);
    for (auto selected_index : piano_roll.current_state.selected_indicies)
    {
        piano_roll.current_state.inputs[selected_index].y = y;
        piano_roll.current_state.inputs[selected_index].x = x;
        ListView_Update(piano_roll.lv_hwnd, selected_index);
    }
    SetWindowRedraw(piano_roll.lv_hwnd, true);
    piano_roll.inputs_different = true;
    RedrawWindow(piano_roll.joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    goto def;
}

/**
 * Called when the selection in the piano roll listview changes.
 */
static void on_piano_roll_selection_changed()
{
    piano_roll.current_state.selected_indicies = get_listview_selection(piano_roll.lv_hwnd);
    update_groupbox_status_text();
    RedrawWindow(piano_roll.joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

/**
 * The window procedure for the listview control.
 */
static LRESULT CALLBACK list_view_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
{
    switch (msg)
    {
    case WM_CONTEXTMENU: {
        if (piano_roll.lv_ignore_context_menu)
        {
            piano_roll.lv_ignore_context_menu = false;
            break;
        }

        HMENU h_menu = CreatePopupMenu();
        const auto base_style = piano_roll.readwrite ? MF_ENABLED : MF_DISABLED;
        AppendMenu(h_menu, MF_STRING, 1, L"Copy\tCtrl+C");
        AppendMenu(h_menu, base_style | MF_STRING, 2, L"Paste\tCtrl+V");
        AppendMenu(h_menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(h_menu, base_style | MF_STRING, 3, L"Undo\tCtrl+Z");
        AppendMenu(h_menu, base_style | MF_STRING, 4, L"Redo\tCtrl+Y");
        AppendMenu(h_menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(h_menu, base_style | MF_STRING, 5, L"Clear\tBackspace");
        AppendMenu(h_menu, base_style | MF_STRING, 6, L"Delete\tDelete");
        AppendMenu(h_menu, base_style | MF_STRING, 7, L"Insert frame\tInsert");

        const int offset =
            TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hwnd, 0);

        switch (offset)
        {
        case 1:
            copy_inputs();
            break;
        case 2:
            paste_inputs(false);
            break;
        case 3:
            undo();
            break;
        case 4:
            redo();
            break;
        case 5:
            clear_inputs_in_selection();
            break;
        case 6:
            delete_inputs_in_selection();
            break;
        case 7:
            insert_frames(1);
            break;
        default:
            break;
        }

        break;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
        LVHITTESTINFO lplvhtti{};
        GetCursorPos(&lplvhtti.pt);
        ScreenToClient(hwnd, &lplvhtti.pt);
        ListView_SubItemHitTest(hwnd, &lplvhtti);

        if (lplvhtti.iItem < 0 || lplvhtti.iItem >= piano_roll.current_state.inputs.size())
        {
            g_view_logger->info("[PianoRoll] iItem out of range");
            goto def;
        }

        // Don't start a dragging operation if we're trying to modify read-only inputs
        if (!piano_roll.readwrite && lplvhtti.iSubItem >= 4)
        {
            break;
        }

        piano_roll.lv_dragging = true;
        piano_roll.lv_drag_column = lplvhtti.iSubItem;

        // Case for button dragging: store some info about the clicked button
        if (lplvhtti.iSubItem >= 4)
        {
            auto input = piano_roll.current_state.inputs[lplvhtti.iItem];

            piano_roll.lv_drag_initial_value = !get_input_value_from_column_index(input, piano_roll.lv_drag_column);
            piano_roll.lv_drag_unset = GetKeyState(VK_RBUTTON) & 0x100;
            piano_roll.lv_ignore_context_menu = GetKeyState(VK_RBUTTON) & 0x100;
        }

        goto handle_mouse_move;
    }
    case WM_MOUSEMOVE:
        goto handle_mouse_move;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        if (piano_roll.lv_dragging)
        {
            piano_roll.lv_dragging = false;
            apply_input_buffer();
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, list_view_proc, sId);
        break;
    case WM_KEYDOWN: {
        if (wParam == VK_BACK)
        {
            clear_inputs_in_selection();
            break;
        }

        if (wParam == VK_DELETE)
        {
            delete_inputs_in_selection();
            break;
        }

        if (wParam == VK_INSERT)
        {
            insert_frames(1);
            break;
        }

        if (!(GetKeyState(VK_CONTROL) & 0x8000))
        {
            break;
        }

        if (wParam == 'C')
        {
            copy_inputs();
            break;
        }

        if (wParam == 'V')
        {
            paste_inputs(GetKeyState(VK_SHIFT) & 0x8000);
            break;
        }

        if (wParam == 'Z')
        {
            undo();
            break;
        }

        if (wParam == 'Y')
        {
            redo();
            break;
        }

        break;
    }

    default:
        break;
    }

def:
    return DefSubclassProc(hwnd, msg, wParam, lParam);

handle_mouse_move:

    // Disable dragging if the corresponding mouse button was released. More reliable to do this here instead of
    // MOUSE_XBUTTONDOWN.
    const bool prev_lv_dragging = piano_roll.lv_dragging;

    if (!piano_roll.lv_drag_unset && !(GetKeyState(VK_LBUTTON) & 0x100))
    {
        piano_roll.lv_dragging = false;
    }

    if (piano_roll.lv_drag_unset && !(GetKeyState(VK_RBUTTON) & 0x100))
    {
        piano_roll.lv_dragging = false;
    }

    if (!piano_roll.lv_dragging)
    {
        if (prev_lv_dragging)
        {
            apply_input_buffer();
        }
        goto def;
    }

    LVHITTESTINFO lplvhtti{};
    GetCursorPos(&lplvhtti.pt);
    ScreenToClient(hwnd, &lplvhtti.pt);
    ListView_SubItemHitTest(hwnd, &lplvhtti);

    if (lplvhtti.iItem < 0 || lplvhtti.iItem >= piano_roll.current_state.inputs.size())
    {
        g_view_logger->info("[PianoRoll] iItem out of range");
        goto def;
    }

    // Case for dragging the playhead: seek to the desired frame
    if (piano_roll.lv_drag_column == 0)
    {
        if (!can_seek())
        {
            goto def;
        }

        ThreadPool::submit_task([=] { g_main_ctx.core_ctx->vcr_begin_seek(std::to_string(lplvhtti.iItem), true); });
        return 0;
    }

    if (!piano_roll.readwrite)
    {
        goto def;
    }

    if (!g_config.piano_roll_constrain_edit_to_column && lplvhtti.iSubItem < 4)
    {
        goto def;
    }

    // During a drag operation, we just mutate the input vector in memory and update the listview without doing anything
    // with the core. Only when the drag ends do we actually apply the changes to the core via begin_warp_modify

    const auto column = g_config.piano_roll_constrain_edit_to_column ? piano_roll.lv_drag_column : lplvhtti.iSubItem;
    const auto new_value = piano_roll.lv_drag_unset ? 0 : piano_roll.lv_drag_initial_value;

    SetWindowRedraw(piano_roll.lv_hwnd, false);

    set_input_value_from_column_index(&piano_roll.current_state.inputs[lplvhtti.iItem], column, new_value);
    ListView_Update(hwnd, lplvhtti.iItem);

    // If we are editing a row inside the selection, we want to apply the same modify operation to the other selected
    // rows.
    if (std::ranges::find(piano_roll.current_state.selected_indicies, lplvhtti.iItem) !=
        piano_roll.current_state.selected_indicies.end())
    {
        for (const auto &i : piano_roll.current_state.selected_indicies)
        {
            set_input_value_from_column_index(&piano_roll.current_state.inputs[i], column, new_value);
            ListView_Update(hwnd, i);
        }
    }

    SetWindowRedraw(piano_roll.lv_hwnd, true);
    piano_roll.inputs_different = true;

    goto def;
}

/**
 * The window procedure for the piano roll dialog.
 */
static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG: {
        // We create all the child controls here because windows dialog scaling would mess our stuff up when mixing
        // dialog manager and manual creation
        piano_roll.hwnd = hwnd;
        piano_roll.joy_hwnd = CreateWindowEx(WS_EX_STATICEDGE, JOYSTICK_CLASS, L"", WS_CHILD | WS_VISIBLE, 17, 30, 131,
                                             131, piano_roll.hwnd, nullptr, g_main_ctx.hinst, nullptr);
        CreateWindowEx(0, WC_STATIC, L"History", WS_CHILD | WS_VISIBLE | WS_GROUP | SS_LEFT | SS_CENTERIMAGE, 17, 166,
                       131, 15, piano_roll.hwnd, nullptr, g_main_ctx.hinst, nullptr);
        piano_roll.hist_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTBOX, L"",
                                              WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
                                              17, 186, 131, 181, piano_roll.hwnd, nullptr, g_main_ctx.hinst, nullptr);
        piano_roll.status_hwnd = CreateWindowEx(0, WC_STATIC, L"", WS_CHILD | WS_VISIBLE | WS_GROUP | SS_LEFT, 17, 370,
                                                131, 60, piano_roll.hwnd, nullptr, g_main_ctx.hinst, nullptr);

        // Some controls don't get the font set by default, so we do it manually
        EnumChildWindows(
            hwnd,
            [](HWND hwnd, LPARAM font) {
                SendMessage(hwnd, WM_SETFONT, (WPARAM)font, 0);
                return TRUE;
            },
            SendMessage(hwnd, WM_GETFONT, 0, 0));

        const auto lv_style = WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_ALIGNTOP | LVS_NOSORTHEADER |
                              LVS_SHOWSELALWAYS | LVS_OWNERDATA;

        RECT grid_rect = get_window_rect_client_space(hwnd, GetDlgItem(hwnd, IDC_LIST_PIANO_ROLL));

        piano_roll.lv_hwnd =
            CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, lv_style, grid_rect.left, grid_rect.top,
                           grid_rect.right - grid_rect.left, grid_rect.bottom - grid_rect.top, hwnd,
                           (HMENU)IDC_PIANO_ROLL_LV, g_main_ctx.hinst, nullptr);
        SetWindowSubclass(piano_roll.lv_hwnd, list_view_proc, 0, 0);

        ListView_SetExtendedListViewStyle(piano_roll.lv_hwnd,
                                          LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        HIMAGELIST image_list = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
        ImageList_AddIcon(image_list, LoadIcon(g_main_ctx.hinst, MAKEINTRESOURCE(IDI_CURRENT)));
        ImageList_AddIcon(image_list, LoadIcon(g_main_ctx.hinst, MAKEINTRESOURCE(IDI_MARKER)));
        ListView_SetImageList(piano_roll.lv_hwnd, image_list, LVSIL_SMALL);

        LVCOLUMN lv_column{};
        lv_column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lv_column.fmt = LVCFMT_CENTER;

        // HACK: Insert and then delete dummy column to have all columns center-aligned
        // https://learn.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-lvcolumnw#remarks
        lv_column.cx = 1;
        lv_column.pszText = const_cast<LPWSTR>(L"");
        ListView_InsertColumn(piano_roll.lv_hwnd, 0, &lv_column);

        lv_column.cx = 26;
        lv_column.pszText = const_cast<LPWSTR>(L"");
        ListView_InsertColumn(piano_roll.lv_hwnd, 1, &lv_column);

        lv_column.cx = 65;
        lv_column.pszText = const_cast<LPWSTR>(L"Frame");
        ListView_InsertColumn(piano_roll.lv_hwnd, 2, &lv_column);

        lv_column.cx = 40;
        lv_column.pszText = const_cast<LPWSTR>(L"X");
        ListView_InsertColumn(piano_roll.lv_hwnd, 3, &lv_column);
        lv_column.pszText = const_cast<LPWSTR>(L"Y");
        ListView_InsertColumn(piano_roll.lv_hwnd, 4, &lv_column);

        lv_column.cx = 30;
        for (int i = 4; i <= 15; ++i)
        {
            lv_column.pszText = const_cast<LPWSTR>(get_button_name_from_column_index(i));
            ListView_InsertColumn(piano_roll.lv_hwnd, i + 1, &lv_column);
        }

        ListView_DeleteColumn(piano_roll.lv_hwnd, 0);

        // Manually call all the setup-related callbacks
        update_inputs();
        on_task_changed(g_main_ctx.core_ctx->vcr_get_task());

        const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

        // ReSharper disable once CppRedundantCastExpression
        on_current_sample_changed(static_cast<int32_t>(info.current_sample));
        update_groupbox_status_text();
        update_history_listbox();

        ResizeAnchor::add_anchors(
            piano_roll.hwnd,
            {
                {piano_roll.lv_hwnd, ResizeAnchor::FULL_ANCHOR},
                {piano_roll.joy_hwnd, ResizeAnchor::AnchorFlags::Left | ResizeAnchor::AnchorFlags::Top},
                {piano_roll.hist_hwnd, ResizeAnchor::AnchorFlags::Left | ResizeAnchor::AnchorFlags::Top},
            });

        break;
    }
    case WM_DESTROY:
        g_view_logger->info("[PianoRoll] Unsubscribing from {} messages...", piano_roll.unsubscribe_funcs.size());
        for (auto unsubscribe_func : piano_roll.unsubscribe_funcs)
        {
            unsubscribe_func();
        }

        EnumChildWindows(
            hwnd,
            [](HWND hwnd, LPARAM) {
                DestroyWindow(hwnd);
                return TRUE;
            },
            0);
        piano_roll.lv_hwnd = nullptr;
        piano_roll.hist_hwnd = nullptr;
        piano_roll.hwnd = nullptr;
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        default:
            break;
        }

        if ((HWND)lParam == piano_roll.hist_hwnd && HIWORD(wParam) == LBN_SELCHANGE)
        {
            auto index = ListBox_GetCurSel(piano_roll.hist_hwnd);

            if (index < 0 || index >= piano_roll.piano_roll_history.size())
            {
                break;
            }

            piano_roll.state_index = index;
            set_piano_roll_state(piano_roll.piano_roll_history[index]);
        }
        break;
    case WM_NOTIFY: {
        if (wParam != IDC_PIANO_ROLL_LV)
        {
            break;
        }

        switch (((LPNMHDR)lParam)->code)
        {
        case LVN_ITEMCHANGED: {
            const auto nmlv = (NMLISTVIEW *)lParam;

            if ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_SELECTED)
            {
                on_piano_roll_selection_changed();
            }

            break;
        }
        case LVN_ODSTATECHANGED:
            on_piano_roll_selection_changed();
            break;
        case LVN_GETDISPINFO: {
            const auto plvdi = (NMLVDISPINFO *)lParam;

            if (plvdi->item.iItem < 0 || plvdi->item.iItem >= piano_roll.current_state.inputs.size())
            {
                g_view_logger->info("[PianoRoll] iItem out of range");
                break;
            }

            if (!(plvdi->item.mask & LVIF_TEXT))
            {
                break;
            }

            auto input = piano_roll.current_state.inputs[plvdi->item.iItem];

            switch (plvdi->item.iSubItem)
            {
            case 0: {
                if (piano_roll.current_sample == plvdi->item.iItem)
                {
                    plvdi->item.iImage = 0;
                }
                else if (piano_roll.seek_savestate_frames.contains(plvdi->item.iItem))
                {
                    plvdi->item.iImage = 1;
                }
                else
                {
                    plvdi->item.iImage = 999;
                }

                break;
            }
            case 1:
                StrNCpy(plvdi->item.pszText, std::to_wstring(plvdi->item.iItem).c_str(), plvdi->item.cchTextMax);
                break;
            case 2:
                StrNCpy(plvdi->item.pszText, std::to_wstring(input.x).c_str(), plvdi->item.cchTextMax);
                break;
            case 3:
                StrNCpy(plvdi->item.pszText, std::to_wstring(input.y).c_str(), plvdi->item.cchTextMax);
                break;
            default: {
                auto value = get_input_value_from_column_index(input, plvdi->item.iSubItem);
                auto name = get_button_name_from_column_index(plvdi->item.iSubItem);
                StrNCpy(plvdi->item.pszText, value ? name : L"", plvdi->item.cchTextMax);
                break;
            }
            }
        }
        break;
        }

        break;
    }
    default:
        break;
    }
    return FALSE;
}

void PianoRoll::init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)joystick_proc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.lpszClassName = JOYSTICK_CLASS;
    RegisterClass(&wndclass);
}

void PianoRoll::show()
{
    if (piano_roll.hwnd)
    {
        BringWindowToTop(piano_roll.hwnd);
        return;
    }

    piano_roll.unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed));
    piano_roll.unsubscribe_funcs.push_back(
        Messenger::subscribe(Messenger::Message::CurrentSampleChanged, on_current_sample_changed));
    piano_roll.unsubscribe_funcs.push_back(
        Messenger::subscribe(Messenger::Message::UnfreezeCompleted, on_unfreeze_completed));
    piano_roll.unsubscribe_funcs.push_back(
        Messenger::subscribe(Messenger::Message::WarpModifyStatusChanged, on_warp_modify_status_changed));
    piano_roll.unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::SeekCompleted, on_seek_completed));
    piano_roll.unsubscribe_funcs.push_back(
        Messenger::subscribe(Messenger::Message::SeekSavestateChanged, on_seek_savestate_changed));
    piano_roll.unsubscribe_funcs.push_back(
        Messenger::subscribe(Messenger::Message::EmuPausedChanged, on_emu_paused_changed));

    piano_roll.hwnd = CreateDialog(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_PIANO_ROLL), g_main_ctx.hwnd, dialog_proc);
    ShowWindow(piano_roll.hwnd, SW_SHOW);
}

HWND PianoRoll::hwnd()
{
    return piano_roll.hwnd;
}
