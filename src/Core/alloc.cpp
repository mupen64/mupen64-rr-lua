/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <alloc.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#include <unistd.h>
#include <sys/mman.h>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif


#endif



// https://github.com/mupen64plus/mupen64plus-core/blob/e170c409fb006aa38fd02031b5eefab6886ec125/src/device/r4300/recomp.c#L995

#if defined(__linux__)
static std::mutex page_size_map_lock;
static std::unordered_map<void*, size_t> page_alloc_sizes;
#endif

void *malloc_exec(size_t size)
{
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif defined(__linux__)
    void* block = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED)
        return NULL;

    // allocation succeeded
    {
        std::scoped_lock lock(page_size_map_lock);
        page_alloc_sizes.emplace(block, size);
    }
    return block;
#else
#error "malloc_exec not implemented for this platform"
#endif
}

void *realloc_exec(void *ptr, size_t oldsize, size_t newsize)
{
    void *block = malloc_exec(newsize);
    if (block != NULL)
    {
        size_t copysize;
        copysize = (oldsize < newsize) ? oldsize : newsize;
        memcpy(block, ptr, copysize);
    }
    free_exec(ptr);
    return block;
}

void free_exec(void *ptr)
{
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(__linux__)
    size_t len = 0;
    {
        std::scoped_lock lock(page_size_map_lock);

        auto map_node = page_alloc_sizes.find(ptr);
        assert(map_node != page_alloc_sizes.end());

        len = map_node->second;
        page_alloc_sizes.erase(map_node);
    }
    munmap(ptr, len);
#else
#error "free_exec not implemented for this platform"
#endif
}
