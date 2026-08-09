
#if defined(__linux__)
#include "decan.hpp"

#include <mutex>
#include <stdexcept>

#include <dlfcn.h>
#include <elf.h>
#include <link.h>

static std::mutex g_dl_lock;

namespace decan
{
library::library(const std::filesystem::path &fileName) : m_filename(fileName.string()), m_handle(nullptr)
{
    std::scoped_lock lock(g_dl_lock);
    void *res = dlopen(fileName.c_str(), RTLD_NOW);
    if (res == nullptr)
    {
        throw dll_error(dlerror());
    }
    m_handle = res;
}
library::~library()
{
    if (m_handle) dlclose(m_handle);
}
void *library::get(const char *symbol) const
{
    std::scoped_lock lock(g_dl_lock);
    dlerror();
    void *res = dlsym(m_handle, symbol);

    if (char *err = dlerror(); err != nullptr) throw dll_error(err);
    return res;
}
} // namespace decan

#endif