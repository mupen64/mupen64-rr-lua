/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <ConfigDialog.hpp>
#include <Main.hpp>
#include <NewConfig.hpp>
#include <GamepadManager.hpp>
#include <WinDarkMode.h>

const auto editbox_ids = {IDC_E_A, IDC_E_B, IDC_E_START, IDC_E_ZTRIG, IDC_E_LTRIG, IDC_E_RTRIG, IDC_E_DPLEFT,
    IDC_E_DPRIGHT, IDC_E_DPUP, IDC_E_DPDOWN, IDC_E_CLEFT, IDC_E_CRIGHT, IDC_E_CUP, IDC_E_CDOWN, IDC_EAS_UP,
    IDC_EAS_RIGHT, IDC_EAS_LEFT, IDC_EAS_DOWN, IDC_E_MAG1, IDC_E_MAG2};

const auto button_ids = {IDC_B_A, IDC_B_B, IDC_B_START, IDC_B_ZTRIG, IDC_B_LTRIG, IDC_B_RTRIG, IDC_B_DPLEFT,
    IDC_B_DPRIGHT, IDC_B_DPUP, IDC_B_DPDOWN, IDC_B_CLEFT, IDC_B_CRIGHT, IDC_B_CUP, IDC_B_CDOWN, IDC_BAS_UP,
    IDC_BAS_RIGHT, IDC_BAS_LEFT, IDC_BAS_DOWN, IDC_B_MAG1, IDC_B_MAG2};

struct config_dialog_context
{
    HWND hwnd{};
    HWND devices_hwnd{};
    InputConfig prev_config{};
    size_t selected_controller{};
    std::variant<std::monostate, ButtonMapping *, AxisMapping *> target_value{};
    bool positive_target_axis{};
};

static config_dialog_context g_ctx;

template <size_t N> static void copy_string(char (&dst)[N], const char *src)
{
    strcpy_s(dst, N, src);
}

static std::string virtual_keycode_to_string(int k)
{
    char buf2[64]{};
    if ((k >= 0x30 && k <= 0x39) || (k >= 0x41 && k <= 0x5A))
        sprintf(buf2, "%c", static_cast<char>(k));
    else if (k >= VK_F1 && k <= VK_F24)
        sprintf(buf2, "F%d", k - (VK_F1 - 1));
    else if (k >= VK_NUMPAD0 && k <= VK_NUMPAD9)
        sprintf(buf2, "Num%d", k - VK_NUMPAD0);
    else
        switch (k)
        {
        case VK_LBUTTON:
            copy_string(buf2, "LMB");
            break;
        case VK_RBUTTON:
            copy_string(buf2, "RMB");
            break;
        case VK_MBUTTON:
            copy_string(buf2, "MMB");
            break;
        case VK_XBUTTON1:
            copy_string(buf2, "XMB1");
            break;
        case VK_XBUTTON2:
            copy_string(buf2, "XMB2");
            break;
        case VK_SPACE:
            copy_string(buf2, "Space");
            break;
        case VK_BACK:
            copy_string(buf2, "Backspace");
            break;
        case VK_TAB:
            copy_string(buf2, "Tab");
            break;
        case VK_CLEAR:
            copy_string(buf2, "Clear");
            break;
        case VK_RETURN:
            copy_string(buf2, "Enter");
            break;
        case VK_PAUSE:
            copy_string(buf2, "Pause");
            break;
        case VK_CAPITAL:
            copy_string(buf2, "Caps");
            break;
        case VK_PRIOR:
            copy_string(buf2, "PageUp");
            break;
        case VK_NEXT:
            copy_string(buf2, "PageDn");
            break;
        case VK_END:
            copy_string(buf2, "End");
            break;
        case VK_HOME:
            copy_string(buf2, "Home");
            break;
        case VK_LEFT:
            copy_string(buf2, "Left");
            break;
        case VK_UP:
            copy_string(buf2, "Up");
            break;
        case VK_RIGHT:
            copy_string(buf2, "Right");
            break;
        case VK_DOWN:
            copy_string(buf2, "Down");
            break;
        case VK_SELECT:
            copy_string(buf2, "Select");
            break;
        case VK_PRINT:
            copy_string(buf2, "Print");
            break;
        case VK_SNAPSHOT:
            copy_string(buf2, "PrintScrn");
            break;
        case VK_INSERT:
            copy_string(buf2, "Insert");
            break;
        case VK_DELETE:
            copy_string(buf2, "Delete");
            break;
        case VK_HELP:
            copy_string(buf2, "Help");
            break;
        case VK_MULTIPLY:
            copy_string(buf2, "Num*");
            break;
        case VK_ADD:
            copy_string(buf2, "Num+");
            break;
        case VK_SUBTRACT:
            copy_string(buf2, "Num-");
            break;
        case VK_DECIMAL:
            copy_string(buf2, "Num.");
            break;
        case VK_DIVIDE:
            copy_string(buf2, "Num/");
            break;
        case VK_NUMLOCK:
            copy_string(buf2, "NumLock");
            break;
        case VK_SCROLL:
            copy_string(buf2, "ScrollLock");
            break;
        case /*VK_OEM_PLUS*/ 0xBB:
            copy_string(buf2, "=+");
            break;
        case /*VK_OEM_MINUS*/ 0xBD:
            copy_string(buf2, "-_");
            break;
        case /*VK_OEM_COMMA*/ 0xBC:
            copy_string(buf2, ",");
            break;
        case /*VK_OEM_PERIOD*/ 0xBE:
            copy_string(buf2, ".");
            break;
        case VK_OEM_7:
            copy_string(buf2, "'\"");
            break;
        case VK_OEM_6:
            copy_string(buf2, "]}");
            break;
        case VK_OEM_5:
            copy_string(buf2, "\\|");
            break;
        case VK_OEM_4:
            copy_string(buf2, "[{");
            break;
        case VK_OEM_3:
            copy_string(buf2, "`~");
            break;
        case VK_OEM_2:
            copy_string(buf2, "/?");
            break;
        case VK_OEM_1:
            copy_string(buf2, ";:");
            break;
        default:
            sprintf(buf2, "(%d)", k);
            break;
        }
    return buf2;
}

static std::string hat_direction_name(int32_t mask)
{
    switch (mask)
    {
    case SDL_HAT_UP:
        return "Up";
    case SDL_HAT_RIGHT:
        return "Right";
    case SDL_HAT_DOWN:
        return "Down";
    case SDL_HAT_LEFT:
        return "Left";
    case SDL_HAT_RIGHTUP:
        return "Up+Right";
    case SDL_HAT_RIGHTDOWN:
        return "Down+Right";
    case SDL_HAT_LEFTUP:
        return "Up+Left";
    case SDL_HAT_LEFTDOWN:
        return "Down+Left";
    default:
        return std::to_string(mask);
    }
}

static void update_editbox(int id, const ButtonMapping &mapping)
{
    if (mapping.hat >= 0)
    {
        SetDlgItemText(
            g_ctx.hwnd, id, std::format("Hat {} {}", mapping.hat, hat_direction_name(mapping.hat_mask)).c_str());
        return;
    }

    if (mapping.axis != SDL_GAMEPAD_AXIS_INVALID)
    {
        std::string str;
        if (GamepadManager::current_device_is_gamepad())
        {
            const char *name = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)mapping.axis);
            str = name ? name : std::format("Axis {}", mapping.axis);
        }
        else
        {
            str = std::format("Axis {}", mapping.axis);
        }
        if (mapping.axis_direction < 0) str += " -";
        if (mapping.axis_direction > 0) str += " +";
        SetDlgItemText(g_ctx.hwnd, id, str.c_str());
        return;
    }

    if (mapping.button != SDL_GAMEPAD_BUTTON_INVALID)
    {
        std::string str;
        if (GamepadManager::current_device_is_gamepad())
        {
            const char *name = SDL_GetGamepadStringForButton((SDL_GamepadButton)mapping.button);
            str = name ? name : std::format("Button {}", mapping.button);
        }
        else
        {
            str = std::format("Button {}", mapping.button);
        }
        SetDlgItemText(g_ctx.hwnd, id, str.c_str());
        return;
    }

    if (mapping.key != 0)
    {
        SetDlgItemText(g_ctx.hwnd, id, virtual_keycode_to_string(mapping.key).c_str());
        return;
    }
}

static void update_editbox(int id_negative, int id_positive, const AxisMapping &mapping)
{
    if (mapping.axis != SDL_GAMEPAD_AXIS_INVALID)
    {
        std::string str;
        if (GamepadManager::current_device_is_gamepad())
        {
            const char *name = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)mapping.axis);
            str = name ? name : std::format("Axis {}", mapping.axis);
        }
        else
        {
            str = std::format("Axis {}", mapping.axis);
        }
        SetDlgItemText(g_ctx.hwnd, id_negative, str.c_str());
        SetDlgItemText(g_ctx.hwnd, id_positive, str.c_str());
        return;
    }

    if (mapping.key_negative != 0)
    {
        SetDlgItemText(g_ctx.hwnd, id_negative, virtual_keycode_to_string(mapping.key_negative).c_str());
    }

    if (mapping.key_positive != 0)
    {
        SetDlgItemText(g_ctx.hwnd, id_positive, virtual_keycode_to_string(mapping.key_positive).c_str());
    }
}

static void update_visuals()
{
    for (const auto btn : editbox_ids)
    {
        SetDlgItemText(g_ctx.hwnd, btn, "");
    }

    const auto controller_config = new_config.controller_config[g_ctx.selected_controller];

    update_editbox(IDC_E_A, controller_config.a);
    update_editbox(IDC_E_B, controller_config.b);
    update_editbox(IDC_E_START, controller_config.start);

    update_editbox(IDC_E_ZTRIG, controller_config.z);
    update_editbox(IDC_E_LTRIG, controller_config.l);
    update_editbox(IDC_E_RTRIG, controller_config.r);

    update_editbox(IDC_E_DPLEFT, controller_config.dpad_left);
    update_editbox(IDC_E_DPRIGHT, controller_config.dpad_right);
    update_editbox(IDC_E_DPUP, controller_config.dpad_up);
    update_editbox(IDC_E_DPDOWN, controller_config.dpad_down);

    update_editbox(IDC_E_CLEFT, controller_config.c_left);
    update_editbox(IDC_E_CRIGHT, controller_config.c_right);
    update_editbox(IDC_E_CUP, controller_config.c_up);
    update_editbox(IDC_E_CDOWN, controller_config.c_down);

    update_editbox(IDC_EAS_LEFT, IDC_EAS_RIGHT, controller_config.x);
    update_editbox(IDC_EAS_UP, IDC_EAS_DOWN, controller_config.y);

    update_editbox(IDC_E_MAG1, controller_config.mag1);
    update_editbox(IDC_E_MAG2, controller_config.mag2);

    if (GetFocus() != GetDlgItem(g_ctx.hwnd, IDC_E_MAG1_VALUE))
    {
        SetDlgItemText(g_ctx.hwnd, IDC_E_MAG1_VALUE, std::to_string(controller_config.mag1_val).c_str());
    }
    if (GetFocus() != GetDlgItem(g_ctx.hwnd, IDC_E_MAG2_VALUE))
    {
        SetDlgItemText(g_ctx.hwnd, IDC_E_MAG2_VALUE, std::to_string(controller_config.mag2_val).c_str());
    }

    CheckDlgButton(g_ctx.hwnd, IDC_CHECKACTIVE, new_config.controller_active[g_ctx.selected_controller]);
    CheckDlgButton(g_ctx.hwnd, IDC_CHECKMEMPAK, new_config.controller_mempak[g_ctx.selected_controller]);
}

static bool is_editing()
{
    return !std::holds_alternative<std::monostate>(g_ctx.target_value);
}

static void end_edit()
{
    g_ctx.target_value = std::monostate{};
    update_visuals();
}

static void pre_begin_edit(int edit_id)
{
    if (is_editing())
    {
        end_edit();
    }

    SetDlgItemText(g_ctx.hwnd, edit_id, "...");
}

static void begin_edit(int edit_id, ButtonMapping *ptr)
{
    pre_begin_edit(edit_id);

    g_ctx.target_value = ptr;
}

static void begin_edit(int edit_id, AxisMapping *ptr)
{
    pre_begin_edit(edit_id);

    g_ctx.target_value = ptr;
}

#define HANDLE_EDIT_BEGIN(btn_id, editbox_id, ptr)                                                                     \
    case btn_id:                                                                                                       \
        begin_edit(editbox_id, ptr);                                                                                   \
        SetFocus(GetDlgItem(g_ctx.hwnd, btn_id));                                                                      \
        break;

static LRESULT CALLBACK hotkey_button_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, hotkey_button_subclass_proc, id);
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_CHAR:
        return TRUE;
    case WM_KILLFOCUS:
        end_edit();
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (new_config.preferred_device_guid.has_value()) break;

        if (auto *mapping = std::get_if<ButtonMapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = SDL_GAMEPAD_BUTTON_INVALID;
            (*mapping)->axis = SDL_GAMEPAD_AXIS_INVALID;
            (*mapping)->axis_direction = 0;
            (*mapping)->hat = -1;
            (*mapping)->hat_mask = SDL_HAT_CENTERED;
            (*mapping)->key = wparam;
            end_edit();
        }

        if (auto *mapping = std::get_if<AxisMapping *>(&g_ctx.target_value))
        {
            (*mapping)->axis = SDL_GAMEPAD_AXIS_INVALID;
            if (g_ctx.positive_target_axis)
                (*mapping)->key_positive = wparam;
            else
                (*mapping)->key_negative = wparam;
            end_edit();
        }

        return TRUE;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static void refresh_device_list()
{
    const auto &reg = GamepadManager::device_registry();
    ListBox_ResetContent(g_ctx.devices_hwnd);
    for (const auto &device : reg.devices)
    {
        ListBox_AddString(g_ctx.devices_hwnd, device.name.c_str());
    }

    if (!new_config.preferred_device_guid.has_value())
    {
        ListBox_SetCurSel(g_ctx.devices_hwnd, 0);
        return;
    }

    const auto current_id = GamepadManager::current_gamepad_id();
    for (size_t i = 0; i < ListBox_GetCount(g_ctx.devices_hwnd); i++)
    {
        const auto &device = reg.devices[i];
        const bool current = current_id.has_value() && device.instance_id == current_id;
        const bool same_path = !current_id.has_value() && new_config.preferred_device_path.has_value() &&
                               device.path == new_config.preferred_device_path;
        const bool legacy_guid = !current_id.has_value() && !new_config.preferred_device_path.has_value() &&
                                 device.guid == new_config.preferred_device_guid;
        if (!current && !same_path && !legacy_guid) continue;
        ListBox_SetCurSel(g_ctx.devices_hwnd, i);
        break;
    }
}

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto controller_config = &new_config.controller_config[g_ctx.selected_controller];

    switch (msg)
    {
    case WM_INITDIALOG: {
        g_ctx.hwnd = hwnd;
        g_ctx.devices_hwnd = GetDlgItem(hwnd, IDC_LDEVICES);
        update_visuals();

        for (const auto btn : button_ids)
        {
            SetWindowSubclass(GetDlgItem(hwnd, btn), hotkey_button_subclass_proc, 0, 0);
        }

        const auto cb_hwnd = GetDlgItem(hwnd, IDC_COMBOCONT);
        for (size_t i = 0; i < 4; i++)
        {
            ComboBox_AddString(cb_hwnd, std::format("Controller {}", i + 1).c_str());
        }
        ComboBox_SetCurSel(cb_hwnd, g_ctx.selected_controller);

        refresh_device_list();

        WinDarkMode::attach(hwnd, {.is_dialog = true});
        break;
    }
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            new_config = g_ctx.prev_config;
            EndDialog(hwnd, IDCANCEL);
            break;
        case IDC_B_CLEAR:
            new_config = InputConfig{};
            update_visuals();
            break;
        case IDC_COMBOCONT: {
            if (HIWORD(wparam) != CBN_SELCHANGE) break;

            const auto index = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBOCONT));

            if (index == CB_ERR) break;

            g_ctx.selected_controller = index;
            update_visuals();
            break;
        }
        case IDC_CHECKACTIVE: {
            const auto checked = IsDlgButtonChecked(g_ctx.hwnd, IDC_CHECKACTIVE) == BST_CHECKED;
            new_config.controller_active[g_ctx.selected_controller] = checked;
            break;
        }
        case IDC_CHECKMEMPAK: {
            const auto checked = IsDlgButtonChecked(g_ctx.hwnd, IDC_CHECKMEMPAK) == BST_CHECKED;
            new_config.controller_mempak[g_ctx.selected_controller] = checked;
            break;
        }

            HANDLE_EDIT_BEGIN(IDC_B_A, IDC_E_A, &controller_config->a)
            HANDLE_EDIT_BEGIN(IDC_B_B, IDC_E_B, &controller_config->b)
            HANDLE_EDIT_BEGIN(IDC_B_START, IDC_E_START, &controller_config->start)

            HANDLE_EDIT_BEGIN(IDC_B_ZTRIG, IDC_E_ZTRIG, &controller_config->z)
            HANDLE_EDIT_BEGIN(IDC_B_LTRIG, IDC_E_LTRIG, &controller_config->l)
            HANDLE_EDIT_BEGIN(IDC_B_RTRIG, IDC_E_RTRIG, &controller_config->r)
            HANDLE_EDIT_BEGIN(IDC_B_DPLEFT, IDC_E_DPLEFT, &controller_config->dpad_left)
            HANDLE_EDIT_BEGIN(IDC_B_DPRIGHT, IDC_E_DPRIGHT, &controller_config->dpad_right)
            HANDLE_EDIT_BEGIN(IDC_B_DPUP, IDC_E_DPUP, &controller_config->dpad_up)
            HANDLE_EDIT_BEGIN(IDC_B_DPDOWN, IDC_E_DPDOWN, &controller_config->dpad_down)
            HANDLE_EDIT_BEGIN(IDC_B_CLEFT, IDC_E_CLEFT, &controller_config->c_left)
            HANDLE_EDIT_BEGIN(IDC_B_CRIGHT, IDC_E_CRIGHT, &controller_config->c_right)
            HANDLE_EDIT_BEGIN(IDC_B_CUP, IDC_E_CUP, &controller_config->c_up)
            HANDLE_EDIT_BEGIN(IDC_B_CDOWN, IDC_E_CDOWN, &controller_config->c_down)
            HANDLE_EDIT_BEGIN(IDC_B_MAG1, IDC_E_MAG1, &controller_config->mag1)
            HANDLE_EDIT_BEGIN(IDC_B_MAG2, IDC_E_MAG2, &controller_config->mag2)
        case IDC_E_MAG1_VALUE: {
            if (HIWORD(wparam) == EN_CHANGE)
            {
                char str[16]{};
                GetDlgItemText(g_ctx.hwnd, LOWORD(wparam), str, std::size(str));
                try
                {
                    controller_config->mag1_val = static_cast<uint32_t>(std::clamp(std::stol(str), 0l, 128l));
                }
                catch (...)
                {
                }
            }
            else if (HIWORD(wparam) == EN_KILLFOCUS)
            {
                SetDlgItemText(g_ctx.hwnd, LOWORD(wparam), std::to_string(controller_config->mag1_val).c_str());
            }
        }
        break;
        case IDC_E_MAG2_VALUE: {
            if (HIWORD(wparam) == EN_CHANGE)
            {
                char str[16]{};
                GetDlgItemText(g_ctx.hwnd, LOWORD(wparam), str, std::size(str));
                try
                {
                    controller_config->mag2_val = static_cast<uint32_t>(std::clamp(std::stol(str), 0l, 128l));
                }
                catch (...)
                {
                }
            }
            else if (HIWORD(wparam) == EN_KILLFOCUS)
            {
                SetDlgItemText(g_ctx.hwnd, LOWORD(wparam), std::to_string(controller_config->mag2_val).c_str());
            }
        }
        break;
        case IDC_BAS_LEFT:
            begin_edit(IDC_EAS_LEFT, &controller_config->x);
            g_ctx.positive_target_axis = false;
            break;
        case IDC_BAS_RIGHT:
            begin_edit(IDC_EAS_RIGHT, &controller_config->x);
            g_ctx.positive_target_axis = true;
            break;
        case IDC_BAS_UP:
            begin_edit(IDC_EAS_UP, &controller_config->y);
            g_ctx.positive_target_axis = false;
            break;
        case IDC_BAS_DOWN:
            begin_edit(IDC_EAS_DOWN, &controller_config->y);
            g_ctx.positive_target_axis = true;
            break;
        case IDC_LDEVICES: {
            if (HIWORD(wparam) == LBN_SELCHANGE)
            {
                const auto index = ListBox_GetCurSel(g_ctx.devices_hwnd);
                if (index == LB_ERR) break;

                const auto &reg = GamepadManager::device_registry();
                const auto &device = reg.devices[index];
                new_config.preferred_device_guid = index == 0 ? std::nullopt : device.guid;
                new_config.preferred_device_path = index == 0 ? std::nullopt : device.path;

                if (device.instance_id.has_value())
                    GamepadManager::select_gamepad(*device.instance_id);
                else
                    GamepadManager::update_current_gamepad();
                update_visuals();
            }
            break;
        }
        default:
            break;
        }
        break;
    case WM_DESTROY:
        g_ctx.hwnd = nullptr;
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lparam)->code)
        {
        case BCN_DROPDOWN: {
            const auto nmbcdd = (NMBCDROPDOWN *)lparam;
            if (nmbcdd->hdr.idFrom == IDC_PRESETS)
            {
                POINT pt{};
                GetCursorPos(&pt);

                HMENU h_menu = CreatePopupMenu();
                AppendMenu(h_menu, MF_STRING, 1, "Gamepad");
                AppendMenu(h_menu, MF_STRING, 2, "Keyboard");
                AppendMenu(h_menu, MF_SEPARATOR, 3, "");
                AppendMenu(h_menu, MF_STRING, 4, "Save");
                AppendMenu(h_menu, MF_STRING, 5, "Load");
                const int clicked = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, hwnd, nullptr);

                switch (clicked)
                {
                case 1:
                    new_config.controller_config[g_ctx.selected_controller] = ControllerConfig::gamepad_config();
                    update_visuals();
                    break;
                case 2:
                    new_config.controller_config[g_ctx.selected_controller] = ControllerConfig::keyboard_config();
                    update_visuals();
                    break;
                case 4: {
                    const auto controller_config = new_config.controller_config[g_ctx.selected_controller];
                    const auto json = nlohmann::json(controller_config);
                    const auto json_str = json.dump();

                    std::span<uint8_t> bytes(
                        reinterpret_cast<uint8_t *>(const_cast<char *>(json_str.data())), json_str.size());

                    const auto path = WinFilePicker::show_save_dialog(hwnd, "*.json");
                    if (path.empty()) break;

                    IOUtils::write_entire_file(path, bytes);

                    break;
                }
                case 5: {
                    const auto path = WinFilePicker::show_open_dialog(hwnd, "*.json");
                    if (path.empty()) break;

                    const auto buf = IOUtils::read_entire_file(path);
                    const auto json_str = std::string(reinterpret_cast<const char *>(buf.data()), buf.size());

                    const auto json = nlohmann::json::parse(json_str);
                    if (!json.is_object()) break;

                    const auto controller_config = json.get<ControllerConfig>();
                    new_config.controller_config[g_ctx.selected_controller] = controller_config;
                    update_visuals();
                    break;
                }
                }

                return TRUE;
            }
            break;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void ConfigDialog::show(HWND parent)
{
    load_config();

    g_ctx.prev_config = new_config;

    DialogBox(g_inst, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)dlgproc);

    save_config();
}

void ConfigDialog::on_sdl_event(const SDL_Event &e)
{
    if (e.type == SDL_EVENT_JOYSTICK_ADDED || e.type == SDL_EVENT_JOYSTICK_REMOVED ||
        e.type == SDL_EVENT_GAMEPAD_ADDED || e.type == SDL_EVENT_GAMEPAD_REMOVED ||
        e.type == SDL_EVENT_KEYBOARD_ADDED || e.type == SDL_EVENT_KEYBOARD_REMOVED)
    {
        if (g_ctx.hwnd) refresh_device_list();
    }

    if (!is_editing()) return;

    const auto selected_id = GamepadManager::current_gamepad_id();
    if (!selected_id.has_value()) return;

    const bool is_gamepad = GamepadManager::current_device_is_gamepad();

    const auto capture_button = [](int32_t button) {
        if (auto *mapping = std::get_if<ButtonMapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = button;
            (*mapping)->axis = SDL_GAMEPAD_AXIS_INVALID;
            (*mapping)->axis_direction = 0;
            (*mapping)->hat = -1;
            (*mapping)->hat_mask = SDL_HAT_CENTERED;
            (*mapping)->key = 0;
            end_edit();
        }
    };

    const auto capture_axis = [](int32_t axis, int16_t value) {
        if (std::abs(value) <= AXIS_THRESHOLD) return;

        if (auto *mapping = std::get_if<AxisMapping *>(&g_ctx.target_value))
        {
            (*mapping)->axis = axis;
            (*mapping)->key_negative = 0;
            (*mapping)->key_positive = 0;
            end_edit();
        }
        else if (auto *mapping = std::get_if<ButtonMapping *>(&g_ctx.target_value))
        {
            (*mapping)->axis = axis;
            (*mapping)->axis_direction = value < 0 ? -1 : 1;
            (*mapping)->button = SDL_GAMEPAD_BUTTON_INVALID;
            (*mapping)->hat = -1;
            (*mapping)->hat_mask = SDL_HAT_CENTERED;
            (*mapping)->key = 0;
            end_edit();
        }
    };

    if (is_gamepad && e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && e.gbutton.which == *selected_id)
    {
        capture_button(e.gbutton.button);
    }
    else if (is_gamepad && e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && e.gaxis.which == *selected_id)
    {
        capture_axis(e.gaxis.axis, e.gaxis.value);
    }
    else if (!is_gamepad && e.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN && e.jbutton.which == *selected_id)
    {
        capture_button(e.jbutton.button);
    }
    else if (!is_gamepad && e.type == SDL_EVENT_JOYSTICK_AXIS_MOTION && e.jaxis.which == *selected_id)
    {
        capture_axis(e.jaxis.axis, e.jaxis.value);
    }
    else if (!is_gamepad && e.type == SDL_EVENT_JOYSTICK_HAT_MOTION && e.jhat.which == *selected_id &&
             e.jhat.value != SDL_HAT_CENTERED)
    {
        if (auto *mapping = std::get_if<ButtonMapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = SDL_GAMEPAD_BUTTON_INVALID;
            (*mapping)->axis = SDL_GAMEPAD_AXIS_INVALID;
            (*mapping)->axis_direction = 0;
            (*mapping)->hat = e.jhat.hat;
            (*mapping)->hat_mask = e.jhat.value;
            (*mapping)->key = 0;
            end_edit();
        }
    }
}
