/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/Config.hpp>
#include <components/CrashManager.hpp>

typedef struct StacktraceInfo
{
    void *rtl_stacktrace[32]{};
};

static StacktraceInfo stacktrace_info;

#define E(x) {x, #x}
const std::unordered_map<int, std::string> EXCEPTION_NAMES = {
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

static std::filesystem::path get_minidump_path()
{
    return IOUtils::exe_path().parent_path() / "logs" / "mupen.dmp";
}

void create_minidump(EXCEPTION_POINTERS *e)
{
    MINIDUMP_EXCEPTION_INFORMATION info{};

    const auto minidump_path = get_minidump_path();
    const HANDLE h_dump_file = CreateFile(
        minidump_path.string().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    info.ThreadId = GetCurrentThreadId();
    info.ExceptionPointers = e;
    info.ClientPointers = TRUE;

    if (!MiniDumpWriteDump(
            GetCurrentProcess(), GetCurrentProcessId(), h_dump_file, MiniDumpWithDataSegs, &info, NULL, NULL))
    {
        g_view_logger->error("Couldn't create minidump (error code: {})", GetLastError());
    }

    CloseHandle(h_dump_file);
}

/**
 * \brief Gets additional information about the exception address
 * \param addr The address where the exception occurred
 * \return A string containing further information about the exception address
 */
static std::string get_metadata_for_exception_address(void *addr)
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
                char modname[MAX_PATH]{};
                GetModuleBaseName(h_process, maxbase, modname, std::size(modname));
            }
        }

        char modname[MAX_PATH]{};
        if (GetModuleBaseName(h_process, maxbase, modname, std::size(modname)))
        {
            return std::format("Address: {:#08x} (closest: {} {:#08x})", (uintptr_t)addr, modname, (uintptr_t)maxbase);
        }
    }

    // Whatever, module search failed so just return the absolute minimum
    return std::format("Address: {:#08x}", (uintptr_t)addr);
}

static std::string get_exception_code_friendly_name(const _EXCEPTION_POINTERS *e)
{
    std::string exception_name = EXCEPTION_NAMES.contains(e->ExceptionRecord->ExceptionCode)
                                     ? EXCEPTION_NAMES.at(e->ExceptionRecord->ExceptionCode)
                                     : "Unknown exception";

    return std::format("{} ({:#08x})", exception_name, e->ExceptionRecord->ExceptionCode);
}

static __forceinline void fill_stacktrace_info()
{
    stacktrace_info = {};
    stacktrace_info.rtl_stacktrace[0] = nullptr;
    CaptureStackBackTrace(0, std::size(stacktrace_info.rtl_stacktrace), stacktrace_info.rtl_stacktrace, NULL);
}

static void log_crash(const std::string &additional_exception_info)
{
    SYSTEMTIME time;
    GetSystemTime(&time);

    g_view_logger->critical("Crash!");
    g_view_logger->critical(get_mupen_name());
    g_view_logger->critical(std::format("{:02}/{:02}/{} {:02}:{:02}:{:02}", time.wDay, time.wMonth, time.wYear,
        time.wHour, time.wMinute, time.wSecond));
    g_view_logger->critical("Video: {}", g_config.selected_video_plugin);
    g_view_logger->critical("Audio: {}", g_config.selected_audio_plugin);
    g_view_logger->critical("Input: {}", g_config.selected_input_plugin);
    g_view_logger->critical("RSP: {}", g_config.selected_rsp_plugin);
    g_view_logger->critical("VCR Task: {}", static_cast<int>(g_main_ctx.CoreCtx->vcr_get_task()));
    g_view_logger->critical("Core Executing: {}", g_main_ctx.CoreCtx->vr_get_launched());
    g_view_logger->critical(additional_exception_info);

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
        TaskDialog(g_main_ctx.hwnd, g_main_ctx.hinst, L"Error", L"An error has occured",
            L"Crash dumps have been automatically generated. You can choose to continue program execution.",
            TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);
    }
    else
    {
        TaskDialog(g_main_ctx.hwnd, g_main_ctx.hinst, L"Error", L"An error has occured",
            L"Crash dumps have been automatically generated. The program will now exit.", TDCBF_CLOSE_BUTTON,
            TD_ERROR_ICON, &result);
    }

    return result == IDCLOSE;
}

LONG WINAPI exception_handler(_EXCEPTION_POINTERS *e)
{
    fill_stacktrace_info();

    std::string exception_info;
    exception_info += get_metadata_for_exception_address(e->ExceptionRecord->ExceptionAddress) + " " +
                      get_exception_code_friendly_name(e) + " ";
    exception_info += "(from SetUnhandledExceptionFilter) ";
    log_crash(exception_info);

    create_minidump(e);

    const bool is_continuable = !(e->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

    const bool close = show_crash_dialog(is_continuable);

    return close ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_EXECUTION;
}

void invalid_parameter_handler(
    const wchar_t *expression, const wchar_t *function, const wchar_t *file, unsigned int line, uintptr_t)
{
    fill_stacktrace_info();

    std::string exception_info;
    exception_info += std::format("File: {} ", file ? IOUtils::to_utf8_string(file) : "(unknown)");
    exception_info += std::format("Function: {} ", function ? IOUtils::to_utf8_string(function) : "(unknown)");
    exception_info += std::format("Expression: {} ", expression ? IOUtils::to_utf8_string(expression) : "(unknown)");
    exception_info += std::format("Line: {} ", line);
    exception_info += "(from _set_invalid_parameter_handler) ";

    log_crash(exception_info);
    show_crash_dialog(false);
}

// See https://randomascii.wordpress.com/2012/07/05/when-even-crashing-doesnt-work/
static void enable_crashing_on_crashes()
{
    const DWORD EXCEPTION_SWALLOWING = 0x1;
    typedef BOOL(WINAPI * tGetPolicy)(LPDWORD lpFlags);
    typedef BOOL(WINAPI * tSetPolicy)(DWORD dwFlags);

    const HMODULE k32 = GetModuleHandle("kernel32.dll");
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
