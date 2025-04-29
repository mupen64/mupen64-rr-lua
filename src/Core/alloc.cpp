#include "stdafx.h"
#include <alloc.h>

#ifdef WIN32
#include <Windows.h>
#endif

// https://github.com/mupen64plus/mupen64plus-core/blob/e170c409fb006aa38fd02031b5eefab6886ec125/src/device/r4300/recomp.c#L995

void* malloc_exec(size_t size)
{
#ifdef WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
#error "malloc_exec not implemented for this platform"
#endif
}

void* realloc_exec(void* ptr, size_t oldsize, size_t newsize)
{
    void* block = malloc_exec(newsize);
    if (block != NULL)
    {
        size_t copysize;
        copysize = (oldsize < newsize)
        ? oldsize
        : newsize;
        memcpy(block, ptr, copysize);
    }
    free_exec(ptr);
    return block;
}

void free_exec(void* ptr)
{
#ifdef WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
#error "free_exec not implemented for this platform"
#endif
}
