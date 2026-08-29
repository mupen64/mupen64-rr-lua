/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "HotkeyUtils.hpp"

const std::unordered_map<uint32_t, SDL_Keycode> WIN_TO_SDL_KEYCODE = {
    {'A', SDLK_A},
    {'B', SDLK_B},
    {'C', SDLK_C},
    {'D', SDLK_D},
    {'E', SDLK_E},
    {'F', SDLK_F},
    {'G', SDLK_G},
    {'H', SDLK_H},
    {'I', SDLK_I},
    {'J', SDLK_J},
    {'K', SDLK_K},
    {'L', SDLK_L},
    {'M', SDLK_M},
    {'N', SDLK_N},
    {'O', SDLK_O},
    {'P', SDLK_P},
    {'Q', SDLK_Q},
    {'R', SDLK_R},
    {'S', SDLK_S},
    {'T', SDLK_T},
    {'U', SDLK_U},
    {'V', SDLK_V},
    {'W', SDLK_W},
    {'X', SDLK_X},
    {'Y', SDLK_Y},
    {'Z', SDLK_Z},

    {'0', SDLK_0},
    {'1', SDLK_1},
    {'2', SDLK_2},
    {'3', SDLK_3},
    {'4', SDLK_4},
    {'5', SDLK_5},
    {'6', SDLK_6},
    {'7', SDLK_7},
    {'8', SDLK_8},
    {'9', SDLK_9},

    {VK_F1, SDLK_F1},
    {VK_F2, SDLK_F2},
    {VK_F3, SDLK_F3},
    {VK_F4, SDLK_F4},
    {VK_F5, SDLK_F5},
    {VK_F6, SDLK_F6},
    {VK_F7, SDLK_F7},
    {VK_F8, SDLK_F8},
    {VK_F9, SDLK_F9},
    {VK_F10, SDLK_F10},
    {VK_F11, SDLK_F11},
    {VK_F12, SDLK_F12},
    {VK_F13, SDLK_F13},
    {VK_F14, SDLK_F14},
    {VK_F15, SDLK_F15},
    {VK_F16, SDLK_F16},
    {VK_F17, SDLK_F17},
    {VK_F18, SDLK_F18},
    {VK_F19, SDLK_F19},
    {VK_F20, SDLK_F20},
    {VK_F21, SDLK_F21},
    {VK_F22, SDLK_F22},
    {VK_F23, SDLK_F23},
    {VK_F24, SDLK_F24},

    {VK_NUMPAD0, SDLK_KP_0},
    {VK_NUMPAD1, SDLK_KP_1},
    {VK_NUMPAD2, SDLK_KP_2},
    {VK_NUMPAD3, SDLK_KP_3},
    {VK_NUMPAD4, SDLK_KP_4},
    {VK_NUMPAD5, SDLK_KP_5},
    {VK_NUMPAD6, SDLK_KP_6},
    {VK_NUMPAD7, SDLK_KP_7},
    {VK_NUMPAD8, SDLK_KP_8},
    {VK_NUMPAD9, SDLK_KP_9},

    {VK_MULTIPLY, SDLK_KP_MULTIPLY},
    {VK_ADD, SDLK_KP_PLUS},
    {VK_SEPARATOR, SDLK_SEPARATOR},
    {VK_SUBTRACT, SDLK_KP_MINUS},
    {VK_DECIMAL, SDLK_KP_PERIOD},
    {VK_DIVIDE, SDLK_KP_DIVIDE},

    {VK_PRIOR, SDLK_PAGEUP},
    {VK_NEXT, SDLK_PAGEDOWN},
    {VK_END, SDLK_END},
    {VK_HOME, SDLK_HOME},
    {VK_LEFT, SDLK_LEFT},
    {VK_UP, SDLK_UP},
    {VK_RIGHT, SDLK_RIGHT},
    {VK_DOWN, SDLK_DOWN},

    {VK_INSERT, SDLK_INSERT},
    {VK_DELETE, SDLK_DELETE},

    {VK_OEM_1, SDLK_SEMICOLON},
    {VK_OEM_PLUS, SDLK_EQUALS},
    {VK_OEM_COMMA, SDLK_COMMA},
    {VK_OEM_MINUS, SDLK_MINUS},
    {VK_OEM_PERIOD, SDLK_PERIOD},
    {VK_OEM_2, SDLK_SLASH},
    {VK_OEM_3, SDLK_GRAVE},
    {VK_OEM_4, SDLK_LEFTBRACKET},
    {VK_OEM_5, SDLK_BACKSLASH},
    {VK_OEM_6, SDLK_RIGHTBRACKET},
    {VK_OEM_7, SDLK_APOSTROPHE},

    {VK_BACK, SDLK_BACKSPACE},
    {VK_TAB, SDLK_TAB},
    {VK_CLEAR, SDLK_CLEAR},
    {VK_RETURN, SDLK_RETURN},
    {VK_SHIFT, SDLK_LSHIFT},
    {VK_CONTROL, SDLK_LCTRL},
    {VK_MENU, SDLK_LALT},
    {VK_PAUSE, SDLK_PAUSE},
    {VK_CAPITAL, SDLK_CAPSLOCK},
    {VK_ESCAPE, SDLK_ESCAPE},
    {VK_SPACE, SDLK_SPACE},

    {VK_LSHIFT, SDLK_LSHIFT},
    {VK_RSHIFT, SDLK_RSHIFT},
    {VK_LCONTROL, SDLK_LCTRL},
    {VK_RCONTROL, SDLK_RCTRL},
    {VK_LMENU, SDLK_LALT},
    {VK_RMENU, SDLK_RALT},

    {VK_LWIN, SDLK_LGUI},
    {VK_RWIN, SDLK_RGUI},
    {VK_APPS, SDLK_APPLICATION},

    {VK_PRINT, SDLK_PRINTSCREEN},
    {VK_SNAPSHOT, SDLK_PRINTSCREEN},
    {VK_HELP, SDLK_HELP},
    {VK_NUMLOCK, SDLK_NUMLOCKCLEAR},
    {VK_SCROLL, SDLK_SCROLLLOCK},
    {VK_SLEEP, SDLK_SLEEP},

    {VK_VOLUME_MUTE, SDLK_MUTE},
    {VK_VOLUME_DOWN, SDLK_VOLUMEDOWN},
    {VK_VOLUME_UP, SDLK_VOLUMEUP},

    {VK_SELECT, SDLK_SELECT},
    {VK_EXECUTE, SDLK_EXECUTE},

    {VK_OEM_102, SDLK_BACKSLASH},

};

struct DialogParams
{
    std::string headline;
    Hotkey hotkey = Hotkey::make_unassigned();
};

std::optional<Hotkey::Trigger> HotkeyUtils::vk_to_trigger(uint32_t vk)
{
    if (vk == 0) return Hotkey::Trigger{std::monostate{}};

    if (WIN_TO_SDL_KEYCODE.contains(vk)) return Hotkey::KeyCode(WIN_TO_SDL_KEYCODE.at(vk));
    if (vk == VK_LBUTTON) return Hotkey::MouseButton(SDL_BUTTON_LEFT);
    if (vk == VK_MBUTTON) return Hotkey::MouseButton(SDL_BUTTON_MIDDLE);
    if (vk == VK_RBUTTON) return Hotkey::MouseButton(SDL_BUTTON_RIGHT);
    if (vk == VK_XBUTTON1) return Hotkey::MouseButton(SDL_BUTTON_X1);
    if (vk == VK_XBUTTON2) return Hotkey::MouseButton(SDL_BUTTON_X2);
    return std::nullopt;
}

std::optional<uint32_t> HotkeyUtils::trigger_to_vk(const Hotkey::Trigger &trigger)
{
    if (std::holds_alternative<std::monostate>(trigger)) return 0;

    if (std::holds_alternative<Hotkey::MouseButton>(trigger))
    {
        const auto mouse_button = std::get<Hotkey::MouseButton>(trigger);
        switch (mouse_button.get())
        {
        case SDL_BUTTON_LEFT:
            return VK_LBUTTON;
        case SDL_BUTTON_MIDDLE:
            return VK_MBUTTON;
        case SDL_BUTTON_RIGHT:
            return VK_RBUTTON;
        case SDL_BUTTON_X1:
            return VK_XBUTTON1;
        case SDL_BUTTON_X2:
            return VK_XBUTTON2;
        default:
            return 0;
        }
    }

    if (std::holds_alternative<Hotkey::KeyCode>(trigger))
    {
        const SDL_Keycode keycode = std::get<Hotkey::KeyCode>(trigger).get();
        for (const auto &[win, sdl] : WIN_TO_SDL_KEYCODE)
        {
            if (sdl == keycode) return win;
        }
        return std::nullopt;
    }

    return std::nullopt;
}

static LRESULT CALLBACK HotkeyButtonSubclassProc(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR ref_data)
{
    const auto params = reinterpret_cast<DialogParams *>(ref_data);

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, HotkeyButtonSubclassProc, id);
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_CHAR:
        return TRUE;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wparam == VK_CONTROL)
        {
            params->hotkey.ctrl = true;
        }
        else if (wparam == VK_SHIFT)
        {
            params->hotkey.shift = true;
        }
        else if (wparam == VK_MENU)
        {
            params->hotkey.alt = true;
        }
        else
        {
            const auto trigger = HotkeyUtils::vk_to_trigger(wparam);
            if (!trigger) break;
            params->hotkey.trigger = *trigger;
            EndDialog(GetParent(hwnd), IDOK);
        }

        SetDlgItemText(GetParent(hwnd), IDC_CURRENT_HOTKEY, params->hotkey.to_string().c_str());

        return TRUE;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    const auto prop_key = "IDD_HOTKEY_Params";
    auto params = static_cast<DialogParams *>(GetProp(hwnd, prop_key));

    switch (msg)
    {
    case WM_INITDIALOG: {
        SetProp(hwnd, prop_key, reinterpret_cast<DialogParams *>(lparam));
        params = reinterpret_cast<DialogParams *>(lparam);

        Static_SetText(GetDlgItem(hwnd, IDC_STATIC), params->headline.c_str());
        SetFocus(GetDlgItem(hwnd, IDC_CURRENT_HOTKEY));

        SetWindowSubclass(
            GetDlgItem(hwnd, IDC_CURRENT_HOTKEY), HotkeyButtonSubclassProc, 0, reinterpret_cast<DWORD_PTR>(params));
        return TRUE;
    }
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        case IDC_CLEAR:
            params->hotkey = Hotkey::make_empty();
            EndDialog(hwnd, IDOK);
            break;
        default:
            break;
        }
        break;
    case WM_MBUTTONDOWN: {
        const auto trigger = HotkeyUtils::vk_to_trigger(VK_MBUTTON);
        if (!trigger) break;
        params->hotkey.trigger = *trigger;
        EndDialog(hwnd, IDOK);
        break;
    }
    case WM_XBUTTONDOWN:
        if (HIWORD(wparam) == XBUTTON1 || HIWORD(wparam) == XBUTTON2)
        {
            const auto trigger = HotkeyUtils::vk_to_trigger(HIWORD(wparam));
            if (!trigger) break;
            params->hotkey.trigger = *trigger;
            EndDialog(hwnd, IDOK);
        }
        break;
    default:
        break;
    }
    return FALSE;
}

bool HotkeyUtils::show_prompt(const HWND hwnd, const std::string &caption, Hotkey &hotkey)
{
    const auto prev_hotkey = hotkey;

    hotkey = Hotkey::make_unassigned();
    auto params = new DialogParams{.headline = caption, .hotkey = hotkey};

    const INT_PTR result =
        DialogBoxParam(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_HOTKEY), hwnd, DlgProc, reinterpret_cast<LPARAM>(params));
    const bool confirmed = result == IDOK;

    if (confirmed)
        hotkey = params->hotkey;
    else
        hotkey = prev_hotkey;

    delete params;

    return confirmed;
}

void HotkeyUtils::try_associate_hotkey(
    const HWND hwnd, const std::string &action, const Hotkey &new_hotkey, const bool through_action_manager)
{
    const auto set_hotkey = [=](const std::string &action, const Hotkey &hotkey) {
        if (through_action_manager)
        {
            ActionManager::associate_hotkey(action, hotkey);
        }
        else
        {
            g_config.hotkeys[action] = hotkey;
        }
    };

    if (new_hotkey.is_empty())
    {
        set_hotkey(action, Hotkey::make_empty());
        return;
    }

    if (g_config.hotkeys.at(action) == new_hotkey)
    {
        return;
    }

    std::vector<std::pair<std::string, Hotkey>> conflicting_hotkeys;

    for (const auto &pair : g_config.hotkeys)
    {
        if (pair.first != action && pair.second == new_hotkey)
        {
            conflicting_hotkeys.emplace_back(pair.first, pair.second);
        }
    }

    if (conflicting_hotkeys.empty())
    {
        set_hotkey(action, new_hotkey);
        return;
    }

    std::string conflicting_hotkey_identifiers;
    for (const auto &action : conflicting_hotkeys | std::views::keys)
    {
        conflicting_hotkey_identifiers += std::format("- {}\n", action);
    }

    const auto str = std::format("The key combination {} is already used by:\n\n{}\nHow would you like to proceed?",
        new_hotkey.to_string(), conflicting_hotkey_identifiers);

    const size_t choice = DialogService::show_multiple_choice_dialog(VIEW_DLG_HOTKEY_CONFLICT,
        {"Keep New", "Keep Old", "Proceed Anyway"}, str, "Hotkey Conflict", CoreMessageTone::Warn, hwnd);

    switch (choice)
    {
    case 0:
        for (const auto &action : conflicting_hotkeys | std::views::keys)
        {
            set_hotkey(action, Hotkey::make_empty());
        }
        set_hotkey(action, new_hotkey);
        break;
    case 1:
        set_hotkey(action, Hotkey::make_empty());
        break;
    case 2:
        set_hotkey(action, new_hotkey);
        break;
    default:
        break;
    }
}
