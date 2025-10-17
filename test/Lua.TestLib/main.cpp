/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cstdio>
extern "C" {
  #include <lua.h>
  #include <lauxlib.h>
}

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALL _cdecl
#else
#define EXPORT
#define CALL
#endif

static int hello_world(lua_State* L) {
  puts("hello, world!");
  return 0;
}

luaL_Reg TESTLIB_FUNCTIONS[] = {
  {"hello_world", hello_world},
  {NULL, NULL}
};

EXPORT int luaopen_testlib(lua_State* L) {
  luaL_newlib(L, TESTLIB_FUNCTIONS);
  lua_setglobal(L, "testlib");
  return 0;
}