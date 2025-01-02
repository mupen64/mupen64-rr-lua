#include <DbgHelp.h>
#include <Windows.h>
#include <core/r4300/Plugin.hpp>
#include <shared/services/PlatformService.h>

#include "shared/services/FrontendService.h"

void *PlatformService::load_library(const wchar_t *path, uint64_t *error) {
  auto module = LoadLibrary(path);
  if (error) {
    *error = GetLastError();
  }
  return module;
}

void PlatformService::free_library(void *module) {
  if (!FreeLibrary((HMODULE)module)) {
    FrontendService::show_dialog(
        std::format(L"Failed to free library {:#06x}.", (unsigned long)module)
            .c_str(),
        L"Core", FrontendService::DialogType::Error);
  }
}

void *PlatformService::get_function_in_module(void *module, const char *name) {
  return GetProcAddress((HMODULE)module, name);
}

void (*PlatformService::get_free_function_in_module(void *module))(void *) {
  auto dll_crt_free = (DLLCRTFREE)GetProcAddress((HMODULE)module, "DllCrtFree");
  if (dll_crt_free != nullptr)
    return dll_crt_free;

  ULONG size;
  auto import_descriptor =
      (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(
          module, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, nullptr);
  if (import_descriptor != nullptr) {
    while (import_descriptor->Characteristics && import_descriptor->Name) {
      auto importDllName = (LPCSTR)((PBYTE)module + import_descriptor->Name);
      auto importDllHandle = GetModuleHandleA(importDllName);
      if (importDllHandle != nullptr) {
        dll_crt_free = (DLLCRTFREE)GetProcAddress(importDllHandle, "free");
        if (dll_crt_free != nullptr)
          return dll_crt_free;
      }

      import_descriptor++;
    }
  }

  // this is probably always wrong
  return free;
}
