#include "CrashHelper.h"
#include <Psapi.h>
#include <core/r4300/r4300.h>
#include <core/r4300/vcr.h>
#include <shared/Config.hpp>
#include <stacktrace>
#include <view/gui/Loggers.h>
#include <view/gui/Main.h>

/**
 * \brief Gets additional information about the exception address
 * \param addr The address where the exception occurred
 * \return A string containing further information about the exception address
 */
std::wstring get_metadata_for_exception_address(void *addr) {
  const HANDLE h_process = GetCurrentProcess();
  DWORD cb_needed;
  HMODULE h_mods[1024];

  if (EnumProcessModules(h_process, h_mods, sizeof(h_mods), &cb_needed)) {
    HMODULE maxbase = nullptr;
    for (int i = 0; i < (int)(cb_needed / sizeof(HMODULE)); i++) {
      if (h_mods[i] > maxbase && h_mods[i] < addr) {
        maxbase = h_mods[i];
        wchar_t modname[MAX_PATH]{};
        GetModuleBaseName(h_process, maxbase, modname, std::size(modname));
      }
    }

    wchar_t modname[MAX_PATH]{};
    if (GetModuleBaseName(h_process, maxbase, modname, std::size(modname))) {
      return std::format(L"Address: {:#08x} (closest: {} {:#08x})",
                         (uintptr_t)addr, modname, (uintptr_t)maxbase);
    }
  }

  // Whatever, module search failed so just return the absolute minimum
  return std::format(L"Address: {:#08x}", (uintptr_t)addr);
}

std::wstring get_exception_code_friendly_name(
    const _EXCEPTION_POINTERS *exception_pointers_ptr) {
  std::wstring str;
  switch (exception_pointers_ptr->ExceptionRecord->ExceptionCode) {
  case EXCEPTION_ACCESS_VIOLATION:
    str = L"Access violation";
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    str = L"Attempted to access out-of-bounds array index";
  case EXCEPTION_FLT_DENORMAL_OPERAND:
    str = L"An operand in floating point operation was denormal";
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    str = L"Float division by zero";
  case EXCEPTION_STACK_OVERFLOW:
    str = L"Stack overflow";
  default:
    break;
  }

  str += std::format(L" ({:#08x})",
                     exception_pointers_ptr->ExceptionRecord->ExceptionCode);
  return str;
}

void CrashHelper::log_crash(_EXCEPTION_POINTERS *exception_pointers_ptr) {
  SYSTEMTIME time;
  GetSystemTime(&time);

  g_view_logger->critical(L"Crash!");
  g_view_logger->critical(MUPEN_VERSION);
  g_view_logger->critical(std::format(L"{:02}/{:02}/{} {:02}:{:02}:{:02}",
                                      time.wDay, time.wMonth, time.wYear,
                                      time.wHour, time.wMinute, time.wSecond));
  g_view_logger->critical(get_metadata_for_exception_address(
      exception_pointers_ptr->ExceptionRecord->ExceptionAddress));
  g_view_logger->critical(
      get_exception_code_friendly_name(exception_pointers_ptr));
  g_view_logger->critical(L"Video: {}", g_config.selected_video_plugin);
  g_view_logger->critical(L"Audio: {}", g_config.selected_audio_plugin);
  g_view_logger->critical(L"Input: {}", g_config.selected_input_plugin);
  g_view_logger->critical(L"RSP: {}", g_config.selected_rsp_plugin);
  g_view_logger->critical(L"VCR Task: {}", static_cast<int>(VCR::get_task()));
  g_view_logger->critical(L"Emu Launched: {}", (bool)emu_launched);

  const std::stacktrace st = std::stacktrace::current();
  g_view_logger->critical("Stacktrace:");
  for (const auto &stacktrace_entry : st) {
    g_view_logger->critical(std::to_string(stacktrace_entry));
  }

  g_view_logger->flush();
}
