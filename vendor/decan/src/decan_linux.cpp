
#if defined(__linux__)
  #include "decan.hpp"

  #include <exception>
  #include <fstream>
  #include <iostream>
  #include <memory>
  #include <stdexcept>
  #include <string>
  #include <unordered_map>

  #include <dlfcn.h>
  #include <elf.h>
  #include <link.h>

namespace decan {
  library::library(const std::filesystem::path& fileName) :
    m_filename(fileName.string()), m_handle([&]() -> void* {
      void* res = dlopen(fileName.c_str(), RTLD_NOW);
      if (res == nullptr) {
        throw std::runtime_error(dlerror());
      }
      return res;
    }()) {}
  library::~library() {
    int res = dlclose(m_handle);
    if (res != 0) {
      std::cerr << "dlclose error: " << dlerror() << '\n';
      std::cerr << "terminating...\n";
      std::terminate();
    }
  }
  void* library::get(const char* symbol) const {
    dlerror();
    void* res = dlsym(m_handle, symbol);
    char* err = dlerror();
    if (err != nullptr) {
      throw std::runtime_error(err);
    }
    return res;
  }
}  // namespace decan

#endif