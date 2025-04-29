#pragma once

void* malloc_exec(size_t size);
void* realloc_exec(void* ptr, size_t oldsize, size_t newsize);
void free_exec(void* ptr);
