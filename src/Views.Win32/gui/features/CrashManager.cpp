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
#include <backward.hpp>

typedef struct StacktraceInfo {
    std::stacktrace stl_stacktrace{};
    void* rtl_stacktrace[32]{};
    std::vector<backward::ResolvedTrace> backward_stacktrace{};
} t_stacktrace_info;

static t_stacktrace_info stacktrace_info;

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

static std::wstring get_exception_code_friendly_name(const _EXCEPTION_POINTERS* e)
{
    std::wstring exception_name = EXCEPTION_NAMES.contains(e->ExceptionRecord->ExceptionCode) ? EXCEPTION_NAMES.at(e->ExceptionRecord->ExceptionCode) : L"Unknown exception";

    return std::format(L"{} ({:#08x})", exception_name, e->ExceptionRecord->ExceptionCode);
}

static __forceinline void fill_stacktrace_info()
{
    stacktrace_info = {};
    stacktrace_info.stl_stacktrace = std::stacktrace::current();
    stacktrace_info.rtl_stacktrace[0] = nullptr;
    CaptureStackBackTrace(0, std::size(stacktrace_info.rtl_stacktrace), stacktrace_info.rtl_stacktrace, NULL);

    backward::StackTrace st;
    st.load_here();

    backward::TraceResolver tr;
    tr.load_stacktrace(st);

    for (size_t i = 0; i < st.size(); ++i)
    {
        stacktrace_info.backward_stacktrace.push_back(tr.resolve(st[i]));
    }
}

static void log_crash(const std::wstring& additional_exception_info)
{
    SYSTEMTIME time;
    GetSystemTime(&time);

    g_view_logger->critical(L"Crash!");
    g_view_logger->critical(get_mupen_name());
    g_view_logger->critical(std::format(L"{:02}/{:02}/{} {:02}:{:02}:{:02}", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond));
    g_view_logger->critical(L"Video: {}", g_config.selected_video_plugin);
    g_view_logger->critical(L"Audio: {}", g_config.selected_audio_plugin);
    g_view_logger->critical(L"Input: {}", g_config.selected_input_plugin);
    g_view_logger->critical(L"RSP: {}", g_config.selected_rsp_plugin);
    g_view_logger->critical(L"VCR Task: {}", static_cast<int>(core_vcr_get_task()));
    g_view_logger->critical(L"Core Executing: {}", core_vr_get_launched());
    g_view_logger->critical(additional_exception_info);

    g_view_logger->critical("STL Stacktrace:");
    for (const auto& stacktrace_entry : stacktrace_info.stl_stacktrace)
    {
        g_view_logger->critical(std::format("{}", std::to_string(stacktrace_entry)));
    }

    g_view_logger->critical("RTL Stacktrace:");
    for (auto i = 0; i < std::size(stacktrace_info.rtl_stacktrace); ++i)
    {
        const auto frame = stacktrace_info.rtl_stacktrace[i];
        if (!frame)
        {
            break;
        }
        g_video_logger->critical(frame);
    }

    g_view_logger->critical("backward-cpp Stacktrace:");
    for (const auto& stacktrace_entry : stacktrace_info.backward_stacktrace)
    {
        g_view_logger->critical(std::format("{} {} {}", stacktrace_entry.object_filename, stacktrace_entry.object_function, stacktrace_entry.addr));
    }

    g_view_logger->flush();
}

bool show_crash_dialog(bool continuable)
{
    if (g_config.silent_mode)
    {
        return true;
    }

    int result = 0;
    if (continuable)
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

    return result == IDCLOSE;
}

LONG WINAPI exception_handler(_EXCEPTION_POINTERS* e)
{
    fill_stacktrace_info();

    std::wstring exception_info;
    exception_info += get_metadata_for_exception_address(e->ExceptionRecord->ExceptionAddress) + L" " + get_exception_code_friendly_name(e) + L" ";
    exception_info += L"(from SetUnhandledExceptionFilter) ";
    log_crash(exception_info);

    const bool is_continuable = !(e->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

    const bool close = show_crash_dialog(is_continuable);

    return close ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_EXECUTION;
}

void invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t)
{
    fill_stacktrace_info();

    std::wstring exception_info;
    exception_info += std::format(L"File: {} ", file ? file : L"(unknown)");
    exception_info += std::format(L"Function: {} ", function ? function : L"(unknown)");
    exception_info += std::format(L"Expression: {} ", expression ? expression : L"(unknown)");
    exception_info += std::format(L"Line: {} ", line);
    exception_info += L"(from _set_invalid_parameter_handler) ";

    log_crash(exception_info);
    show_crash_dialog(false);
}

// See https://randomascii.wordpress.com/2012/07/05/when-even-crashing-doesnt-work/
static void enable_crashing_on_crashes()
{
    const DWORD EXCEPTION_SWALLOWING = 0x1;
    typedef BOOL(WINAPI * tGetPolicy)(LPDWORD lpFlags);
    typedef BOOL(WINAPI * tSetPolicy)(DWORD dwFlags);

    const HMODULE k32 = GetModuleHandle(L"kernel32.dll");
    const auto p_get_policy = (tGetPolicy)GetProcAddress(k32, "GetProcessUserModeExceptionPolicy");
    const auto p_set_policy = (tSetPolicy)GetProcAddress(k32, "SetProcessUserModeExceptionPolicy");
    if (p_get_policy && p_set_policy)
    {
        DWORD dwFlags;
        if (p_get_policy(&dwFlags))
        {
            p_set_policy(dwFlags & ~EXCEPTION_SWALLOWING);
        }
    }
    BOOL insanity = FALSE;
    SetUserObjectInformationA(GetCurrentProcess(),
                              UOI_TIMERPROC_EXCEPTION_SUPPRESSION,
                              &insanity, sizeof(insanity));
}

void CrashManager::init()
{
    enable_crashing_on_crashes();

    SetUnhandledExceptionFilter(exception_handler);
    _set_invalid_parameter_handler(invalid_parameter_handler);

    // Some test cases:
    //
    // RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL, NULL);
    //
    // RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);
    //
    // printf(0);
    //
    // In WndProc:
    // int* a = 0; *a = 1;
    //
}
