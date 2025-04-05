/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <gui/Loggers.h>
#include <gui/Main.h>
#include <gui/features/CrashManager.h>

/**
 * \brief Gets additional information about the exception address
 * \param addr The address where the exception occurred
 * \return A string containing further information about the exception address
 */
static std::wstring get_metadata_for_exception_address(void* addr)
{
    const HANDLE h_process = GetCurrentProcess();
    DWORD cb_needed;
    HMODULE h_mods[1024];

    if (EnumProcessModules(h_process, h_mods, sizeof(h_mods), &cb_needed))
    {
        HMODULE maxbase = nullptr;
        for (int i = 0; i < (int)(cb_needed / sizeof(HMODULE)); i++)
        {
            if (h_mods[i] > maxbase && h_mods[i] < addr)
            {
                maxbase = h_mods[i];
                wchar_t modname[MAX_PATH]{};
                GetModuleBaseName(h_process, maxbase, modname, std::size(modname));
            }
        }

        wchar_t modname[MAX_PATH]{};
        if (GetModuleBaseName(h_process, maxbase, modname, std::size(modname)))
        {
            return std::format(L"Address: {:#08x} (closest: {} {:#08x})", (uintptr_t)addr, modname, (uintptr_t)maxbase);
        }
    }

    // Whatever, module search failed so just return the absolute minimum
    return std::format(L"Address: {:#08x}", (uintptr_t)addr);
}

#define E(x) {x, L#x}
const std::unordered_map<int, std::wstring> EXCEPTION_NAMES = {
E(EXCEPTION_ACCESS_VIOLATION),
E(EXCEPTION_ACCESS_VIOLATION),
E(EXCEPTION_DATATYPE_MISALIGNMENT),
E(EXCEPTION_BREAKPOINT),
E(EXCEPTION_SINGLE_STEP),
E(EXCEPTION_ARRAY_BOUNDS_EXCEEDED),
E(EXCEPTION_FLT_DENORMAL_OPERAND),
E(EXCEPTION_FLT_DIVIDE_BY_ZERO),
E(EXCEPTION_FLT_INEXACT_RESULT),
E(EXCEPTION_FLT_INVALID_OPERATION),
E(EXCEPTION_FLT_OVERFLOW),
E(EXCEPTION_FLT_STACK_CHECK),
E(EXCEPTION_FLT_UNDERFLOW),
E(EXCEPTION_INT_DIVIDE_BY_ZERO),
E(EXCEPTION_INT_OVERFLOW),
E(EXCEPTION_PRIV_INSTRUCTION),
E(EXCEPTION_IN_PAGE_ERROR),
E(EXCEPTION_ILLEGAL_INSTRUCTION),
E(EXCEPTION_NONCONTINUABLE_EXCEPTION),
E(EXCEPTION_STACK_OVERFLOW),
E(EXCEPTION_INVALID_DISPOSITION),
E(EXCEPTION_GUARD_PAGE),
E(EXCEPTION_INVALID_HANDLE),
};

#undef E
static std::wstring get_exception_code_friendly_name(const _EXCEPTION_POINTERS* exception_pointers_ptr)
{
    std::wstring exception_name = EXCEPTION_NAMES.contains(exception_pointers_ptr->ExceptionRecord->ExceptionCode) ? EXCEPTION_NAMES.at(exception_pointers_ptr->ExceptionRecord->ExceptionCode) : L"Unknown exception";

    return std::format(L"{} ({:#08x})", exception_name, exception_pointers_ptr->ExceptionRecord->ExceptionCode);
}


static void log_crash(const _EXCEPTION_POINTERS* exception_pointers_ptr)
{
    SYSTEMTIME time;
    GetSystemTime(&time);

    g_view_logger->critical(L"Crash!");
    g_view_logger->critical(get_mupen_name());
    g_view_logger->critical(std::format(L"{:02}/{:02}/{} {:02}:{:02}:{:02}", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond));
    g_view_logger->critical(get_metadata_for_exception_address(exception_pointers_ptr->ExceptionRecord->ExceptionAddress) + L" " + get_exception_code_friendly_name(exception_pointers_ptr));
    g_view_logger->critical(L"Video: {}", g_config.selected_video_plugin);
    g_view_logger->critical(L"Audio: {}", g_config.selected_audio_plugin);
    g_view_logger->critical(L"Input: {}", g_config.selected_input_plugin);
    g_view_logger->critical(L"RSP: {}", g_config.selected_rsp_plugin);
    g_view_logger->critical(L"VCR Task: {}", static_cast<int>(core_vcr_get_task()));
    g_view_logger->critical(L"Core Executing: {}", core_vr_get_launched());

    const std::stacktrace st = std::stacktrace::current();
    g_view_logger->critical("Stacktrace:");
    for (const auto& stacktrace_entry : st)
    {
        g_view_logger->critical(std::format("{}", std::to_string(stacktrace_entry)));
    }

    g_view_logger->flush();
}

LONG WINAPI exception_handler(_EXCEPTION_POINTERS* e)
{
    log_crash(e);

    if (g_config.silent_mode)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    const bool is_continuable = !(e->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

    int result = 0;

    if (is_continuable)
    {
        TaskDialog(g_main_hwnd, g_app_instance, L"Error",
                   L"An error has occured", L"A crash log has been automatically generated. You can choose to continue program execution.",
                   TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);
    }
    else
    {
        TaskDialog(g_main_hwnd, g_app_instance, L"Error",
                   L"An error has occured", L"A crash log has been automatically generated. The program will now exit.", TDCBF_CLOSE_BUTTON, TD_ERROR_ICON,
                   &result);
    }

    if (result == IDCLOSE)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

void CrashManager::init()
{
    SetUnhandledExceptionFilter(exception_handler);

    // raise noncontinuable exception (impossible to recover from it)
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL, NULL);
    //
    // raise continuable exception
    // RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);

    // TODO: Register _set_invalid_parameter_handler, maybe some other handlers too. Fixes cases like printf(0); not triggering the crash dialog.
    // See https://randomascii.wordpress.com/2012/07/05/when-even-crashing-doesnt-work/
}
