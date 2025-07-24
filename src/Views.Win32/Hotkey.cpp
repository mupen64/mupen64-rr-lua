/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Hotkey.h>

bool t_hotkey::is_nothing() const
{
    return !this->ctrl && !this->shift && !this->alt && this->key == 0;
}

std::wstring t_hotkey::to_wstring() const
{
    wchar_t buf[260]{};
    const int k = this->key;

    if (!this->ctrl && !this->shift && !this->alt && !this->key)
    {
        return L"(nothing)";
    }

    if (this->ctrl)
        StrCat(buf, L"Ctrl ");
    if (this->shift)
        StrCat(buf, L"Shift ");
    if (this->alt)
        StrCat(buf, L"Alt ");
    if (k)
    {
        wchar_t buf2[64]{};
        if ((k >= 0x30 && k <= 0x39) || (k >= 0x41 && k <= 0x5A))
            wsprintf(buf2, L"%c", static_cast<char>(k));
        else if (k >= VK_F1 && k <= VK_F24)
            wsprintf(buf2, L"F%d", k - (VK_F1 - 1));
        else if (k >= VK_NUMPAD0 && k <= VK_NUMPAD9)
            wsprintf(buf2, L"Num%d", k - VK_NUMPAD0);
        else
            switch (k)
            {
            case VK_LBUTTON:
                StrCpy(buf2, L"LMB");
                break;
            case VK_RBUTTON:
                StrCpy(buf2, L"RMB");
                break;
            case VK_MBUTTON:
                StrCpy(buf2, L"MMB");
                break;
            case VK_XBUTTON1:
                StrCpy(buf2, L"XMB1");
                break;
            case VK_XBUTTON2:
                StrCpy(buf2, L"XMB2");
                break;
            case VK_SPACE:
                StrCpy(buf2, L"Space");
                break;
            case VK_BACK:
                StrCpy(buf2, L"Backspace");
                break;
            case VK_TAB:
                StrCpy(buf2, L"Tab");
                break;
            case VK_CLEAR:
                StrCpy(buf2, L"Clear");
                break;
            case VK_RETURN:
                StrCpy(buf2, L"Enter");
                break;
            case VK_PAUSE:
                StrCpy(buf2, L"Pause");
                break;
            case VK_CAPITAL:
                StrCpy(buf2, L"Caps");
                break;
            case VK_PRIOR:
                StrCpy(buf2, L"PageUp");
                break;
            case VK_NEXT:
                StrCpy(buf2, L"PageDn");
                break;
            case VK_END:
                StrCpy(buf2, L"End");
                break;
            case VK_HOME:
                StrCpy(buf2, L"Home");
                break;
            case VK_LEFT:
                StrCpy(buf2, L"Left");
                break;
            case VK_UP:
                StrCpy(buf2, L"Up");
                break;
            case VK_RIGHT:
                StrCpy(buf2, L"Right");
                break;
            case VK_DOWN:
                StrCpy(buf2, L"Down");
                break;
            case VK_SELECT:
                StrCpy(buf2, L"Select");
                break;
            case VK_PRINT:
                StrCpy(buf2, L"Print");
                break;
            case VK_SNAPSHOT:
                StrCpy(buf2, L"PrintScrn");
                break;
            case VK_INSERT:
                StrCpy(buf2, L"Insert");
                break;
            case VK_DELETE:
                StrCpy(buf2, L"Delete");
                break;
            case VK_HELP:
                StrCpy(buf2, L"Help");
                break;
            case VK_MULTIPLY:
                StrCpy(buf2, L"Num*");
                break;
            case VK_ADD:
                StrCpy(buf2, L"Num+");
                break;
            case VK_SUBTRACT:
                StrCpy(buf2, L"Num-");
                break;
            case VK_DECIMAL:
                StrCpy(buf2, L"Num.");
                break;
            case VK_DIVIDE:
                StrCpy(buf2, L"Num/");
                break;
            case VK_NUMLOCK:
                StrCpy(buf2, L"NumLock");
                break;
            case VK_SCROLL:
                StrCpy(buf2, L"ScrollLock");
                break;
            case /*VK_OEM_PLUS*/ 0xBB:
                StrCpy(buf2, L"=+");
                break;
            case /*VK_OEM_MINUS*/ 0xBD:
                StrCpy(buf2, L"-_");
                break;
            case /*VK_OEM_COMMA*/ 0xBC:
                StrCpy(buf2, L",");
                break;
            case /*VK_OEM_PERIOD*/ 0xBE:
                StrCpy(buf2, L".");
                break;
            case VK_OEM_7:
                StrCpy(buf2, L"'\"");
                break;
            case VK_OEM_6:
                StrCpy(buf2, L"]}");
                break;
            case VK_OEM_5:
                StrCpy(buf2, L"\\|");
                break;
            case VK_OEM_4:
                StrCpy(buf2, L"[{");
                break;
            case VK_OEM_3:
                StrCpy(buf2, L"`~");
                break;
            case VK_OEM_2:
                StrCpy(buf2, L"/?");
                break;
            case VK_OEM_1:
                StrCpy(buf2, L";:");
                break;
            default:
                wsprintf(buf2, L"(%d)", k);
                break;
            }
        StrCat(buf, buf2);
    }
    return buf;
}
