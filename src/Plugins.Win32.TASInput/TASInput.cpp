/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Combo.hpp>
#include <ConfigDialog.hpp>
#include <GamepadManager.hpp>
#include <JoystickControl.hpp>
#include <Main.hpp>
#include <NewConfig.hpp>
#include <TASInput.hpp>

#define WM_EDIT_END (WM_USER + 3)
#define WM_UPDATE_STATUS (WM_USER + 4)

constexpr auto JOYSTICK_CONTROL_CLASS = "JoystickControl";

enum class ComboTask
{
    Idle,
    Play,
    Record
};

struct Status
{
    /**
     * \brief The instance's UI thread
     */
    HANDLE thread = nullptr;

    /**
     * \brief The initial client rectangle before any style changes are applied
     */
    RECT initial_client_rect;

    /**
     * \brief The initial window rectangle before any style changes are applied
     */
    RECT initial_window_rect;

    /**
     * \brief The window's position. Used for restoring the position after dialog changes and its position is reset by
     * window manager
     */
    POINT window_position{};

    /**
     * \brief The current internal input state before any processing
     */
    CoreButtons current_input{};

    /**
     * \brief The internal input state at the previous GetKeys call before any processing
     */
    CoreButtons last_controller_input{};

    /**
     * \brief Ignores the next joystick increment, used for relative mode tracking
     */
    bool ignore_next_down[2]{};

    /**
     * \brief Ignores the next joystick decrement, used for relative mode tracking
     */
    bool ignore_next_up[2]{};

    /**
     * \brief The index of the currently active combo into the combos array, or -1 if none is active
     */
    int32_t active_combo_index = -1;

    /**
     * \brief The index of the currently renamed combo into the combos array, or -1 if none is being renamed
     */
    int32_t renaming_combo_index = -1;

    /**
     * \brief The frame count relative to the current combo's start
     */
    int64_t combo_frame = 0;

    /**
     * \brief Whether the currently playing combo is paused
     */
    bool combo_paused = false;

    /**
     * \brief Handle of the edit box used for renaming combos
     */
    HWND combo_edit_box = nullptr;

    struct t_set_visuals_request
    {
        CoreButtons input;
        bool needs_processing;
    };

    std::optional<t_set_visuals_request> pending_set_visuals_request{};
    std::mutex pending_visuals_mutex{};

    std::optional<std::string> pending_status{};
    bool status_message_pending{};
    std::mutex pending_status_mutex{};

    std::vector<t_combo> combos{};

    bool last_lmb_down{};
    bool last_rmb_down{};
    CoreButtons autofire_input_a{};
    CoreButtons autofire_input_b{};
    bool ready;
    HWND hwnd{};
    HWND combos_hwnd{};
    HWND joy_hwnd;
    HWND combo_listbox;
    int controller_index;
    ComboTask combo_task = ComboTask::Idle;

    void set_status(const std::string &str);

    bool show_context_menu(int x, int y);

    /**
     * \brief Gets whether a combo is currently active.
     */
    bool combo_active();
    /**
     * \brief Saves the combo list to a file
     */
    void save_combos();

    /**
     * \brief Loads the combo list from a file
     */
    void load_combos(const std::filesystem::path &path);

    void start_edit(int);

    void end_edit(int, char *);

    /**
     * \brief Updates the UI
     * \param input The values to be shown in the UI
     * \param needs_processing Whether the UI values need per-frame processing.
     */
    void set_visuals(CoreButtons input, bool needs_processing = true);

    /**
     * \brief Queues the UI to be updated at the next possible opportunity. Doesn't block the caller until the UI has
     * updated.
     * \param input The values to be shown in the UI
     * \param needs_processing Whether the UI values need per-frame processing.
     */
    void set_visuals_lazy(CoreButtons input, bool needs_processing = true);

    void set_visuals_if_needed();

    /**
     * \brief Processes the input with steps such as autofire or combo overrides
     * \param input The input to process
     * \return The processed input
     */
    CoreButtons get_processed_input(CoreButtons input);

    /**
     * \brief Activates the mupen window, releasing focus capture from the current window
     */
    void activate_emulator_window();

    void on_config_changed();

    void on_timer();

    void get_input(CoreButtons *keys);
};

static std::atomic<int64_t> frame_counter{};
static std::atomic<bool> new_frame{};
static std::atomic<bool> rom_open{};
static std::atomic<bool> s_event_watch_attached{};
static HMENU hmenu{};
static HFONT icon_font{};
static Status status[NUMBER_OF_CONTROLS]{};
static int MOUSE_LBUTTONREDEFINITION = VK_LBUTTON;
static int MOUSE_RBUTTONREDEFINITION = VK_RBUTTON;

static bool event_watch(void *, SDL_Event *event)
{
    GamepadManager::on_sdl_event(*event);
    ConfigDialog::on_sdl_event(*event);
    return true;
}

static void attach_event_watch()
{
    if (s_event_watch_attached) return;
    s_event_watch_attached = true;

    SDL_AddEventWatch(event_watch, nullptr);
}

static void CALLBACK timer_callback(HWND, UINT, UINT_PTR, DWORD);
LRESULT CALLBACK EditBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    switch (msg)
    {
    case WM_GETDLGCODE: {
        if (wParam == VK_RETURN)
        {
            goto apply;
        }
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        break;
    }
    case WM_KILLFOCUS: {
        goto apply;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, EditBoxProc, sId);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);

apply:

    char txt[MAX_PATH]{};
    GetWindowText(hwnd, txt, std::size(txt));
    SendMessage(GetParent(GetParent(hwnd)), WM_EDIT_END, 0, (LPARAM)txt);
    DestroyWindow(hwnd);

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void Status::get_input(CoreButtons *keys)
{
    keys->value = get_processed_input(current_input).value;

    if (combo_task == ComboTask::Play && !combo_paused)
    {
        if (combo_frame >= combos[active_combo_index].samples.size() - 1)
        {
            if (new_config.loop_combo)
            {
                combo_frame = 0;
            }
            else
            {
                set_status("Finished combo");
                combo_task = ComboTask::Idle;
                // Reset input on last frame, or it sticks which feels weird
                // We also need to reprocess the inputs since source data change
                current_input = {0};
                keys->value = get_processed_input(current_input).value;
                goto end;
            }
        }

        set_status(std::format("Playing... ({} / {})", combo_frame + 1, combos[active_combo_index].samples.size() - 1));
        combo_frame++;
    }

end:
    if (combo_task == ComboTask::Record)
    {
        // We process this last, because we need the processed inputs
        combos[active_combo_index].samples.push_back(*keys);
        set_status(std::format("Recording... ({})", combos[active_combo_index].samples.size()));
    }

    set_visuals_lazy(*keys, false);
}

CoreButtons Status::get_processed_input(CoreButtons input)
{
    input.value |= frame_counter % 2 == 0 ? autofire_input_a.value : autofire_input_b.value;

    if (combo_task == ComboTask::Play && !combo_paused)
    {
        auto combo_input = combos[active_combo_index].samples[combo_frame];
        if (!combos[active_combo_index].uses_joystick())
        {
            // We want to use our joystick inputs
            combo_input.x = input.x;
            combo_input.y = input.y;
        }
        input = combo_input;
    }

    return input;
}

void Status::activate_emulator_window()
{
    if (GetFocus() == GetDlgItem(hwnd, IDC_EDITX) || GetFocus() == GetDlgItem(hwnd, IDC_EDITY) ||
        (combo_edit_box != nullptr && GetFocus() == combo_edit_box))
    {
        return;
    }
    SetForegroundWindow(g_plugin->main_window.hwnd());
}

void Status::set_visuals(CoreButtons input, bool needs_processing)
{
    if (!ready) return;

    if (needs_processing)
    {
        input = get_processed_input(input);
    }

    // We don't want to mess with the user's selection
    if (GetFocus() != GetDlgItem(hwnd, IDC_EDITX))
    {
        SetDlgItemText(hwnd, IDC_EDITX, std::to_string(input.x).c_str());
    }

    if (GetFocus() != GetDlgItem(hwnd, IDC_EDITY))
    {
        SetDlgItemText(hwnd, IDC_EDITY, std::to_string(input.y).c_str());
    }

    CheckDlgButton(hwnd, IDC_CHECK_A, input.a);
    CheckDlgButton(hwnd, IDC_CHECK_B, input.b);
    CheckDlgButton(hwnd, IDC_CHECK_START, input.start);
    CheckDlgButton(hwnd, IDC_CHECK_L, input.l);
    CheckDlgButton(hwnd, IDC_CHECK_R, input.r);
    CheckDlgButton(hwnd, IDC_CHECK_Z, input.z);
    CheckDlgButton(hwnd, IDC_CHECK_CUP, input.cu);
    CheckDlgButton(hwnd, IDC_CHECK_CLEFT, input.cl);
    CheckDlgButton(hwnd, IDC_CHECK_CRIGHT, input.cr);
    CheckDlgButton(hwnd, IDC_CHECK_CDOWN, input.cd);
    CheckDlgButton(hwnd, IDC_CHECK_DUP, input.du);
    CheckDlgButton(hwnd, IDC_CHECK_DLEFT, input.dl);
    CheckDlgButton(hwnd, IDC_CHECK_DRIGHT, input.dr);
    CheckDlgButton(hwnd, IDC_CHECK_DDOWN, input.dd);

    JoystickControl::set_position(joy_hwnd, input.x, input.y);
}

void Status::set_visuals_lazy(CoreButtons input, bool needs_processing)
{
    if (!ready) return;

    std::lock_guard lock(pending_visuals_mutex);
    pending_set_visuals_request = t_set_visuals_request{input, needs_processing};
}

void Status::set_visuals_if_needed()
{
    if (!ready) return;
    std::optional<t_set_visuals_request> request;
    {
        std::lock_guard lock(pending_visuals_mutex);
        request = pending_set_visuals_request;
        pending_set_visuals_request.reset();
    }

    if (request.has_value()) set_visuals(request->input, request->needs_processing);
}

static int get_joystick_increment(const bool up)
{
    int increment = up ? 1 : -1;

    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        increment *= 2;
    }

    if (GetKeyState(VK_MENU) & 0x8000)
    {
        increment *= 4;
    }

    return increment;
}

void Status::on_timer()
{
    set_visuals_if_needed();

    CoreButtons controller_input = GamepadManager::get_input(controller_index);

    if (controller_input.value != last_controller_input.value)
    {
        // Input changed, override everything with current
#define BTN(field)                                                                                                     \
    if (controller_input.field && !last_controller_input.field)                                                        \
    {                                                                                                                  \
        current_input.field = 1;                                                                                       \
    }                                                                                                                  \
    if (!controller_input.field && last_controller_input.field)                                                        \
    {                                                                                                                  \
        current_input.field = 0;                                                                                       \
    }
#define JOY(field, i)                                                                                                  \
    if (controller_input.field != last_controller_input.field)                                                         \
    {                                                                                                                  \
        if (controller_input.field > last_controller_input.field)                                                      \
        {                                                                                                              \
            if (ignore_next_down[i])                                                                                   \
                ignore_next_down[i] = false;                                                                           \
            else                                                                                                       \
            {                                                                                                          \
                current_input.field = current_input.field + 5;                                                         \
                ignore_next_up[i] = true;                                                                              \
            }                                                                                                          \
        }                                                                                                              \
        else if (controller_input.field < last_controller_input.field)                                                 \
        {                                                                                                              \
            if (ignore_next_up[i])                                                                                     \
                ignore_next_up[i] = false;                                                                             \
            else                                                                                                       \
            {                                                                                                          \
                current_input.field = current_input.field - 5;                                                         \
                ignore_next_down[i] = true;                                                                            \
            }                                                                                                          \
        }                                                                                                              \
    }
        BTN(dr)
        BTN(dl)
        BTN(dd)
        BTN(du)
        BTN(start)
        BTN(z)
        BTN(b)
        BTN(a)
        BTN(cr)
        BTN(cl)
        BTN(cd)
        BTN(cu)
        BTN(r)
        BTN(l)

        if (new_config.relative_mode)
        {
            JOY(x, 0)
            JOY(y, 1)
        }
        if (!new_config.relative_mode && !new_config.approach_mode &&
            (controller_input.x != last_controller_input.x || controller_input.y != last_controller_input.y))
        {
            current_input.x = controller_input.x;
            current_input.y = controller_input.y;
        }
#undef JOY
#undef BTN
        set_visuals(current_input);
    }

    if (new_config.approach_mode)
    {
        int x = current_input.x;
        int y = current_input.y;

        if (controller_input.x > 0)
            x += 2;
        else if (controller_input.x < 0)
            x -= 2;

        if (controller_input.y > 0)
            y += 2;
        else if (controller_input.y < 0)
            y -= 2;

        current_input.x = std::clamp(x, -128, 127);
        current_input.y = std::clamp(y, -128, 127);
        set_visuals(current_input);
    }
    last_controller_input = controller_input;
}

static void CALLBACK timer_callback(HWND, UINT, UINT_PTR, DWORD)
{
    for (auto &st : status)
    {
        if (st.ready) st.on_timer();
    }
}

INT_PTR CALLBACK combos_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto ctx = reinterpret_cast<Status *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        ctx = reinterpret_cast<Status *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        ctx->combo_listbox = GetDlgItem(hwnd, IDC_MACROLIST);
        ctx->load_combos("combos.cmb");
        break;
    case WM_EDIT_END:
        ctx->end_edit(ctx->renaming_combo_index, (char *)lparam);
        ctx->combo_edit_box = nullptr;
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_PLAY:
            ctx->active_combo_index = ListBox_GetCurSel(ctx->combo_listbox);
            if (ctx->active_combo_index == -1)
            {
                ctx->set_status("No combo selected");
                break;
            }
            ctx->set_status("Playing combo");
            ctx->combo_frame = 0;
            ctx->combo_task = ComboTask::Play;
            break;
        case IDC_STOP:
            ctx->set_status("Idle");
            ctx->combo_task = ComboTask::Idle;
            break;
        case IDC_PAUSE:
            ctx->combo_paused ^= true;
            break;
        case IDC_LOOP:
            new_config.loop_combo ^= true;
            save_config();
            break;
        case IDC_RECORD:
            if (ctx->combo_task == ComboTask::Record)
            {
                ctx->set_status("Recording stopped");
                ctx->combo_task = ComboTask::Idle;
                break;
            }

            ctx->set_status("Recording new combo...");
            ctx->combos.push_back({.name = "Unnamed Combo"});
            ctx->active_combo_index = ListBox_InsertString(ctx->combo_listbox, -1, ctx->combos.back().name.c_str());
            ListBox_SetCurSel(ctx->combo_listbox, ctx->active_combo_index);
            ctx->combo_task = ComboTask::Record;
            break;
        case IDC_EDIT:
            ctx->renaming_combo_index = ListBox_GetCurSel(ctx->combo_listbox);
            if (ctx->renaming_combo_index == -1)
            {
                ctx->set_status("No combo selected");
                break;
            }
            ctx->start_edit(ctx->renaming_combo_index);
            break;
        case IDC_CLEAR:
            ctx->combo_task = ComboTask::Idle;
            ctx->active_combo_index = -1;
            ctx->combos.clear();
            ListBox_ResetContent(ctx->combo_listbox);
            break;
        case IDC_IMPORT: {
            char file[MAX_PATH]{};

            ctx->set_status("Importing...");
            OPENFILENAME data{};
            data.lStructSize = sizeof(data);
            data.lpstrFilter = "Combo file (*.cmb)\0*.cmb\0\0";
            data.nFilterIndex = 1;
            data.nMaxFile = MAX_PATH;
            data.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            data.lpstrFile = file;
            if (GetOpenFileName(&data))
            {
                ctx->load_combos(file);
            }
            ctx->set_status("Imported combo data");
            break;
        }
        case IDC_SAVE:
            ctx->save_combos();
            ctx->set_status("Saved to combos.cmb");
            break;
        default:
            break;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_SETCURSOR:
        SendMessage(ctx->hwnd, msg, wparam, lparam);
        break;
    default:
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto ctx = reinterpret_cast<Status *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    bool lmb_down = GetAsyncKeyState(MOUSE_LBUTTONREDEFINITION) & 0x8000;
    bool rmb_down = GetAsyncKeyState(MOUSE_RBUTTONREDEFINITION) & 0x8000;

    switch (msg)
    {
    case WM_INITDIALOG: {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        ctx = reinterpret_cast<Status *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        ctx->hwnd = hwnd;

        GetClientRect(ctx->hwnd, &ctx->initial_client_rect);
        GetWindowRect(ctx->hwnd, &ctx->initial_window_rect);

        SetWindowPos(ctx->hwnd, nullptr, ctx->window_position.x, ctx->window_position.y, 0, 0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

        SetWindowText(ctx->hwnd, std::format("TASInput - Controller {}", ctx->controller_index + 1).c_str());

        SendDlgItemMessage(ctx->hwnd, IDC_SLIDERX, TBM_SETRANGE, TRUE, MAKELONG(10, 2010));
        SendDlgItemMessage(ctx->hwnd, IDC_SLIDERX, TBM_SETPOS, TRUE,
                           (int)MiscHelpers::remap(new_config.controller_config[ctx->controller_index].x_scale, 0.0f,
                                                   1.0f, 10.0f, 2010.0f));
        SendDlgItemMessage(ctx->hwnd, IDC_SLIDERY, TBM_SETRANGE, TRUE, MAKELONG(10, 2010));
        SendDlgItemMessage(ctx->hwnd, IDC_SLIDERY, TBM_SETPOS, TRUE,
                           (int)MiscHelpers::remap(new_config.controller_config[ctx->controller_index].y_scale, 0.0f,
                                                   1.0f, 10.0f, 2010.0f));

        SetDlgItemText(ctx->hwnd, IDC_X_DOWN, "3");
        SetDlgItemText(ctx->hwnd, IDC_X_UP, "4");
        SetDlgItemText(ctx->hwnd, IDC_Y_DOWN, "6");
        SetDlgItemText(ctx->hwnd, IDC_Y_UP, "5");
        SetDlgItemText(ctx->hwnd, IDC_RESET_JOYSTICK, "\u2022");

        constexpr int controls[] = {IDC_X_DOWN, IDC_X_UP, IDC_Y_DOWN, IDC_Y_UP};
        for (const auto id : controls)
        {
            const auto control = GetDlgItem(hwnd, id);
            SendMessage(control, WM_SETFONT, reinterpret_cast<WPARAM>(icon_font), TRUE);
            RedrawWindow(control, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
        }

        const auto scale = GetDpiForWindow(hwnd) / 96.0;

        ctx->joy_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, JOYSTICK_CONTROL_CLASS, "", WS_CHILD | WS_VISIBLE, 8, 4,
                                       131 * scale, 131 * scale, ctx->hwnd, nullptr, g_inst, nullptr);

        // It can take a bit until we receive the first GetKeys, so let's just show some basic default state in the
        // meanwhile
        ctx->set_visuals(ctx->current_input);

        ctx->on_config_changed();

        ctx->ready = true;
    }
    break;
    case WM_SHOWWINDOW:
        if (!wparam)
        {
            save_config();
        }
        break;
    case SC_MINIMIZE:
        DestroyMenu(hmenu);
        break;
    case WM_DESTROY: {
        ctx->ready = false;
        DestroyWindow(ctx->joy_hwnd);
        ctx->hwnd = nullptr;
    }
    break;
    case JoystickControl::WM_JOYSTICK_POSITION_CHANGED: {
        int x{}, y{};
        JoystickControl::get_position(ctx->joy_hwnd, &x, &y);

        ctx->current_input.x = (int8_t)x;
        ctx->current_input.y = (int8_t)y;

        if (!wparam)
        {
            ctx->set_visuals(ctx->current_input);
        }
        break;
    }
    case JoystickControl::WM_JOYSTICK_DRAG_BEGIN:
        ctx->activate_emulator_window();
        break;
    case WM_CONTEXTMENU:
        if ((HWND)wparam == ctx->hwnd)
        {
            ctx->show_context_menu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        }
        break;
    case WM_LBUTTONDOWN: {
        if (!new_config.client_drag || is_mouse_over_control(ctx->joy_hwnd))
        {
            break;
        }

        ReleaseCapture();
        SendMessage(ctx->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);

        return 0;
    }
    case WM_SETCURSOR: {
        const bool lmb_just_up = !lmb_down && ctx->last_lmb_down;
        const bool rmb_just_up = !rmb_down && ctx->last_rmb_down;
        const bool rmb_just_down = rmb_down && !ctx->last_rmb_down;

        if (lmb_just_up || rmb_just_up)
        {
            // activate mupen window to allow it to get key inputs
            ctx->activate_emulator_window();
        }

        if (rmb_just_down && is_mouse_over_control(ctx->hwnd, IDC_SLIDERX))
        {
            const auto max = SendDlgItemMessage(ctx->hwnd, IDC_SLIDERX, TBM_GETRANGEMAX, 0, 0);
            SendDlgItemMessage(ctx->hwnd, IDC_SLIDERX, TBM_SETPOS, TRUE, max);
        }

        if (rmb_just_down && is_mouse_over_control(ctx->hwnd, IDC_SLIDERY))
        {
            const auto max = SendDlgItemMessage(ctx->hwnd, IDC_SLIDERY, TBM_GETRANGEMAX, 0, 0);
            SendDlgItemMessage(ctx->hwnd, IDC_SLIDERY, TBM_SETPOS, TRUE, max);
        }

        if (rmb_just_down)
        {

#define AUTOFIRE(id, field)                                                                                            \
    {                                                                                                                  \
        if (is_mouse_over_control(ctx->hwnd, id))                                                                      \
        {                                                                                                              \
            if (ctx->autofire_input_a.field || ctx->autofire_input_b.field)                                            \
            {                                                                                                          \
                ctx->autofire_input_a.field = ctx->autofire_input_b.field = 0;                                         \
            }                                                                                                          \
            else                                                                                                       \
            {                                                                                                          \
                if (frame_counter % 2 == 0)                                                                            \
                    ctx->autofire_input_a.field ^= 1;                                                                  \
                else                                                                                                   \
                    ctx->autofire_input_b.field ^= 1;                                                                  \
            }                                                                                                          \
        }                                                                                                              \
    }
            AUTOFIRE(IDC_CHECK_A, a);
            AUTOFIRE(IDC_CHECK_B, b);
            AUTOFIRE(IDC_CHECK_START, start);
            AUTOFIRE(IDC_CHECK_L, l);
            AUTOFIRE(IDC_CHECK_R, r);
            AUTOFIRE(IDC_CHECK_Z, z);
            AUTOFIRE(IDC_CHECK_CUP, cu);
            AUTOFIRE(IDC_CHECK_CLEFT, cl);
            AUTOFIRE(IDC_CHECK_CRIGHT, cr);
            AUTOFIRE(IDC_CHECK_CDOWN, cd);
            AUTOFIRE(IDC_CHECK_DUP, du);
            AUTOFIRE(IDC_CHECK_DLEFT, dl);
            AUTOFIRE(IDC_CHECK_DRIGHT, dr);
            AUTOFIRE(IDC_CHECK_DDOWN, dd);
#undef AUTOFIRE
            ctx->set_visuals(ctx->current_input);
        }

        ctx->last_lmb_down = GetAsyncKeyState(MOUSE_LBUTTONREDEFINITION) & 0x8000;
        ctx->last_rmb_down = GetAsyncKeyState(MOUSE_RBUTTONREDEFINITION) & 0x8000;
    }
    break;
    case WM_NOTIFY: {
        switch (LOWORD(wparam))
        {
        case IDC_SLIDERX:
        case IDC_SLIDERY: {
            const auto id = LOWORD(wparam);
            const auto min = (float)SendDlgItemMessage(ctx->hwnd, id, TBM_GETRANGEMIN, 0, 0);
            const auto max = (float)SendDlgItemMessage(ctx->hwnd, id, TBM_GETRANGEMAX, 0, 0);
            const auto pos = (float)SendDlgItemMessage(ctx->hwnd, id, TBM_GETPOS, 0, 0);
            const auto scale = MiscHelpers::remap(pos, min, max, 0.0f, 1.0f);
            if (id == IDC_SLIDERX)
            {
                new_config.controller_config[ctx->controller_index].x_scale = scale;
            }
            else
            {
                new_config.controller_config[ctx->controller_index].y_scale = scale;
            }
        }
        break;
        default:
            break;
        }
    }
    break;
    case WM_UPDATE_STATUS: {
        std::optional<std::string> status;
        {
            std::lock_guard lock(ctx->pending_status_mutex);
            status = std::move(ctx->pending_status);
            ctx->pending_status.reset();
            ctx->status_message_pending = false;
        }

        if (status.has_value() && ctx->combos_hwnd)
        {
            Static_SetText(GetDlgItem(ctx->combos_hwnd, IDC_STATUS), status->c_str());
        }
        break;
    }
    case WM_SIZE:
    case WM_MOVE: {
        RECT window_rect{};
        GetWindowRect(ctx->hwnd, &window_rect);
        ctx->window_position = {
            window_rect.left,
            window_rect.top,
        };
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_EDITX: {
            if (HIWORD(wparam) != EN_CHANGE)
            {
                break;
            }
            CoreButtons last_input = ctx->current_input;
            char str[8]{};
            GetDlgItemText(ctx->hwnd, LOWORD(wparam), str, std::size(str));
            try
            {
                ctx->current_input.x = std::stol(str);
            }
            catch (...)
            {
            }

            // We don't want an infinite loop, since set_visuals will send IDC_EDITX again
            if (ctx->current_input.x != last_input.x)
            {
                ctx->set_visuals(ctx->current_input);
            }
        }
        break;

        case IDC_EDITY: {
            if (HIWORD(wparam) != EN_CHANGE)
            {
                break;
            }
            CoreButtons last_input = ctx->current_input;
            char str[8]{};
            GetDlgItemText(ctx->hwnd, LOWORD(wparam), str, std::size(str));
            try
            {
                ctx->current_input.y = std::stol(str);
            }
            catch (...)
            {
            }

            // We don't want an infinite loop, since set_visuals will send IDC_EDITX again
            if (ctx->current_input.y != last_input.y)
            {
                ctx->set_visuals(ctx->current_input);
            }
        }
        break;
        case IDC_CLEARINPUT: {
            ctx->current_input = {0};
            ctx->autofire_input_a = {0};
            ctx->autofire_input_b = {0};
            ctx->set_visuals(ctx->current_input);
            break;
        }
        case IDC_RESET_JOYSTICK:
            ctx->current_input.x = 0;
            ctx->current_input.y = 0;
            ctx->set_visuals(ctx->current_input);
            break;
        case IDC_X_DOWN:
        case IDC_X_UP: {
            int increment = get_joystick_increment(LOWORD(wparam) == IDC_X_UP);
            ctx->current_input.x = MiscHelpers::wrapping_clamp(ctx->current_input.x + increment, -128, 127);
            ctx->set_visuals(ctx->current_input);
        }
        break;
        case IDC_Y_DOWN:
        case IDC_Y_UP: {
            int increment = get_joystick_increment(LOWORD(wparam) == IDC_Y_UP);
            ctx->current_input.y = MiscHelpers::wrapping_clamp(ctx->current_input.y + increment, -128, 127);
            ctx->set_visuals(ctx->current_input);
        }
        break;
        case IDC_EXPAND:
            new_config.dialog_expanded[ctx->controller_index] ^= true;
            save_config();
            ctx->on_config_changed();
            break;
#define TOGGLE(field)                                                                                                  \
    {                                                                                                                  \
        ctx->current_input.field = IsDlgButtonChecked(ctx->hwnd, LOWORD(wparam)) ? 1 : 0;                              \
        ctx->autofire_input_a.field = ctx->autofire_input_b.field = 0;                                                 \
    }
        case IDC_CHECK_A:
            TOGGLE(a)
            break;
        case IDC_CHECK_B:
            TOGGLE(b)
            break;
        case IDC_CHECK_START:
            TOGGLE(start)
            break;
        case IDC_CHECK_Z:
            TOGGLE(z)
            break;
        case IDC_CHECK_L:
            TOGGLE(l)
            break;
        case IDC_CHECK_R:
            TOGGLE(r)
            break;
        case IDC_CHECK_CLEFT:
            TOGGLE(cl)
            break;
        case IDC_CHECK_CUP:
            TOGGLE(cu)
            break;
        case IDC_CHECK_CRIGHT:
            TOGGLE(cr)
            break;
        case IDC_CHECK_CDOWN:
            TOGGLE(cd)
            break;
        case IDC_CHECK_DLEFT:
            TOGGLE(dl)
            break;
        case IDC_CHECK_DUP:
            TOGGLE(du)
            break;
        case IDC_CHECK_DRIGHT:
            TOGGLE(dr)
            break;
        case IDC_CHECK_DDOWN:
            TOGGLE(dd)
            break;
#undef TOGGLE
        default:
            break;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static void show_activated_windows()
{
    size_t i = 0;
    for (const auto &st : status)
    {
        ShowWindow(st.hwnd, new_config.controller_active[i] ? SW_SHOW : SW_HIDE);
        i++;
    }
}

bool Status::combo_active()
{
    return active_combo_index != -1;
}

void Status::set_status(const std::string &str)
{
    std::lock_guard lock(pending_status_mutex);
    pending_status = str;

    if (!status_message_pending && hwnd)
    {
        status_message_pending = true;
        PostMessage(hwnd, WM_UPDATE_STATUS, 0, 0);
    }
}

void Status::start_edit(int id)
{
    RECT item_rect;
    ListBox_GetItemRect(combo_listbox, id, &item_rect);
    combo_edit_box = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP, item_rect.left,
                                    item_rect.top, item_rect.right - item_rect.left,
                                    item_rect.bottom - item_rect.top + 4, combo_listbox, 0, g_inst, 0);
    // Clear selection to prevent it from repainting randomly and fighting with our textbox
    ListBox_SetCurSel(combo_listbox, -1);
    SendMessage(combo_edit_box, WM_SETFONT, (WPARAM)SendMessage(combo_listbox, WM_GETFONT, 0, 0), 0);
    SetWindowSubclass(combo_edit_box, EditBoxProc, 0, 0);

    const auto len = ListBox_GetTextLen(combo_listbox, id);
    if (len == LB_ERR) return;
    std::string text(len, '\0');
    ListBox_GetText(combo_listbox, id, text.data());

    SendMessage(combo_edit_box, WM_SETTEXT, 0, (LPARAM)text.c_str());
    PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)combo_edit_box, TRUE);
}

void Status::end_edit(int id, char *name)
{
    if (name != NULL)
    {
        ListBox_DeleteString(combo_listbox, id);

        if (name[0] == '\0')
        {
            combos.erase(combos.begin() + id);
        }
        else
        {
            combos[id].name = name;
            ListBox_InsertString(combo_listbox, id, name);
        }
    }
    set_status("Idle");
}

void Status::save_combos()
{
    const auto path = std::filesystem::path("combos.cmb");

    g_plugin->log_trace(std::format("Saving combos to {}...", path.string()).c_str());

    FILE *f{};
    if (IOUtils::path_fopen_s(f, path, "wb"))
    {
        return;
    }

    const auto serialized = t_combo::serialize_combos(combos);

    (void)fwrite(serialized.data(), sizeof(uint8_t), serialized.size(), f);

    (void)fclose(f);
}

void Status::load_combos(const std::filesystem::path &path)
{
    g_plugin->log_trace(std::format("Loading combos from {}...", path.string()).c_str());

    auto buf = IOUtils::read_entire_file(path);
    if (buf.empty())
    {
        g_plugin->log_error("read_file_buffer failed");
        return;
    }

    combos = t_combo::deserialize_combos(buf);

    ListBox_ResetContent(combo_listbox);
    for (const auto &combo : combos)
    {
        ListBox_InsertString(combo_listbox, -1, combo.name.c_str());
    }
}

bool Status::show_context_menu(int x, int y)
{
    if (is_mouse_over_control(joy_hwnd) || (GetKeyState(MOUSE_LBUTTONREDEFINITION) & 0x8000)) return TRUE;

    const auto prev_config = new_config;

    // HACK: disable topmost so menu doesnt appear under tasinput
    hmenu = CreatePopupMenu();
#define ADD_ITEM(hmenu, x, y) AppendMenu(hmenu, new_config.x ? MF_CHECKED : 0, offsetof(t_config, x), y)
    ADD_ITEM(hmenu, relative_mode, "Relative");
    ADD_ITEM(hmenu, approach_mode, "Approach");
    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    ADD_ITEM(hmenu, always_on_top, "Always on top");
    ADD_ITEM(hmenu, float_from_parent, "Float from parent");
    ADD_ITEM(hmenu, titlebar, "Titlebar");
    ADD_ITEM(hmenu, client_drag, "Client drag");

    int offset = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, hwnd, 0);

    if (offset != 0)
    {
        // offset is the offset into menu config struct of the field which was selected by user, we need to convert it
        // from byte offset to int-width offset
        auto arr = reinterpret_cast<int32_t *>(&new_config);
        arr[offset / sizeof(int32_t)] ^= true;
    }

    // Apply mutually exclusive relative/approach toggle
    if (new_config.relative_mode && !prev_config.relative_mode)
    {
        new_config.approach_mode = false;
    }
    if (new_config.approach_mode && !prev_config.approach_mode)
    {
        new_config.relative_mode = false;
    }

    for (auto &status_dlg : status)
    {
        if (status_dlg.ready && status_dlg.hwnd)
        {
            status_dlg.on_config_changed();
            status_dlg.activate_emulator_window();
        }
    }

    DestroyMenu(hmenu);
    return TRUE;
}

void Status::on_config_changed()
{
    if (new_config.always_on_top)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    set_style(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW, !new_config.float_from_parent);
    set_style(hwnd, GWL_STYLE, DS_SYSMODAL, !new_config.float_from_parent);
    set_style(hwnd, GWL_STYLE, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, new_config.titlebar);

    // HACK: Fix window size when switching between titlebar and no titlebar
    RECT rect = new_config.titlebar ? initial_window_rect : initial_client_rect;
    if (!new_config.titlebar)
    {
        rect.right += 8;
        rect.bottom += 5;
    }
    SetWindowPos(hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE);

    const bool expanded = new_config.dialog_expanded[controller_index];

    if (!expanded)
    {
        DestroyWindow(combos_hwnd);
        combos_hwnd = nullptr;
    }

    if (expanded)
    {
        combos_hwnd = CreateDialogParam(g_inst, MAKEINTRESOURCE(IDD_COMBOS), hwnd, combos_dlgproc, (LPARAM)this);
        CheckDlgButton(combos_hwnd, IDC_LOOP, new_config.loop_combo);

        RECT expanded_rect = rect;
        RECT combos_dlg_rect{};
        GetClientRect(combos_hwnd, &combos_dlg_rect);
        expanded_rect.bottom += combos_dlg_rect.bottom;

        SetWindowPos(hwnd, nullptr, 0, 0, expanded_rect.right, expanded_rect.bottom, SWP_NOMOVE);
        SetWindowPos(combos_hwnd, nullptr, 0, initial_client_rect.bottom, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    SetDlgItemText(hwnd, IDC_EXPAND, expanded ? "Less" : "More");

    save_config();
}

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::Input;

    const auto name = PLUGIN_NAME;
    const auto description = "First-party TAS plugin for Mupen64."
                             "\n"
                             "TAS plugins are not to be distributed separately from Mupen64 and remain tied "
                             "to one version of the emulator."
                             "\n\n"
                             "https://mupen64.com";
    const auto target_version = CURRENT_VERSION;

    auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);
    metadata->name[result.size] = '\0';

    result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);
    metadata->description[result.size] = '\0';

    result = std::format_to_n(metadata->target_version, sizeof(metadata->target_version) - 1, "{}", target_version);
    metadata->target_version[result.size] = '\0';
}

EXPORT void CALL M64RRProcessEvent(Event event)
{
    switch (event.type)
    {
    case M64RRSpec::Event::Type::Initiate: {
        g_plugin = event.initiate.init;

        for (int i = 0; i < 4; ++i)
        {
            g_plugin->controllers[i].present = new_config.controller_active[i];
            g_plugin->controllers[i].raw = false;
            g_plugin->controllers[i].plugin = CoreControllerExtension::None;
            if (new_config.controller_mempak[i]) g_plugin->controllers[i].plugin = CoreControllerExtension::Mempak;
            if (new_config.controller_rumblepak[i])
                g_plugin->controllers[i].plugin = CoreControllerExtension::Rumblepak;
        }

        icon_font = CreateFont(-20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Marlett"));

        // HACK: perform windows left handed mode check
        // and adjust accordingly
        if (GetSystemMetrics(SM_SWAPBUTTON))
        {
            MOUSE_LBUTTONREDEFINITION = VK_RBUTTON;
            MOUSE_RBUTTONREDEFINITION = VK_LBUTTON;
        }

        JoystickControl::register_class(g_inst, JOYSTICK_CONTROL_CLASS);

        save_config();

        break;
    }
    case M64RRSpec::Event::Type::Shutdown: {

        if (icon_font)
        {
            DeleteFont(icon_font);
            icon_font = {};
        }

        if (s_event_watch_attached)
        {
            SDL_RemoveEventWatch(event_watch, nullptr);
            s_event_watch_attached = false;
        }

        break;
    }
    case M64RRSpec::Event::Type::RomOpened: {
        if (!IsWindow(status[0].hwnd))
        {
            for (size_t i = 0; i < 4; ++i)
            {
                status[i].controller_index = i;
                status[i].hwnd = CreateDialogParam(g_inst, MAKEINTRESOURCE(IDD_MAIN), g_plugin->main_window.hwnd(),
                                                   wndproc, reinterpret_cast<LPARAM>(&status[i]));
            }
        }

        attach_event_watch();
        load_config();
        show_activated_windows();
        SetTimer(status[0].hwnd, IDT_TIMER, 16, timer_callback);
        rom_open = true;
        break;
    }
    case M64RRSpec::Event::Type::RomClosed: {
        KillTimer(status[0].hwnd, IDT_TIMER);
        rom_open = false;

        for (auto &st : status)
        {
            ShowWindow(st.hwnd, SW_HIDE);
        }
        break;
    }

    default:
        break;
    }
}

EXPORT void CALL M64RRGetWindows(WindowHandle *windows, size_t *count)
{
    std::vector<WindowHandle> wnds = {status[0].hwnd, status[1].hwnd, status[2].hwnd, status[3].hwnd};
    if (windows) std::memcpy(windows, wnds.data(), sizeof(WindowHandle) * wnds.size());
    if (count) *count = wnds.size();
}

EXPORT void CALL M64RRReadController(int32_t controller, unsigned char *command)
{
    if (controller == -1)
    {
        new_frame = true;
    }
}

EXPORT void CALL M64RRShowConfig(WindowHandle parent_window)
{
    attach_event_watch();
    ConfigDialog::show(parent_window.hwnd());

    // TODO: Do we have to restart the dialogs here like in old version?
}

EXPORT void CALL M64RRGetKeys(int32_t index, CoreButtons *buttons)
{
    if (new_frame)
    {
        ++frame_counter;
        new_frame = false;
    }

    status[index].get_input(buttons);
}

EXPORT void CALL M64RRSetKeys(int32_t index, CoreButtons buttons)
{
    status[index].set_visuals_lazy(buttons, false);
}
