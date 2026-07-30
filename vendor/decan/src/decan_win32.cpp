
#if defined(_WIN32)
  #include "decan.hpp"

  #include <exception>
  #include <fstream>
  #include <iostream>
  #include <memory>
  #include <string>
  #include <system_error>
  #include <unordered_map>

  #define NOMINMAX
  #include <windows.h>

namespace decan {

  library::library(const std::filesystem::path& file_name) :
    m_filename(file_name.string()), m_handle([&]() -> HMODULE {
      HMODULE res = LoadLibraryW(file_name.c_str());
      if (res == nullptr) {
        DWORD last_error = GetLastError();
        throw std::system_error(last_error, std::system_category());
      }
      return res;
    }()) {}

  library::~library() {
    bool good = FreeLibrary(m_handle);
    if (!good) {
      DWORD last_error = GetLastError();
      std::cerr << "FreeLibrary error: "
                << std::system_error(last_error, std::system_category()).what()
                << '\n';
      std::cerr << "terminating...\n";
      std::terminate();
    }
  }

  void* library::get(const char* symbol) const {
    FARPROC res = GetProcAddress(m_handle, symbol);
    if (res == nullptr) {
      DWORD last_error = GetLastError();
      throw std::system_error(last_error, std::system_category());
    }

    return reinterpret_cast<void*>(res);
  }
}  // namespace decan

#endif