extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <core/memory/savestates.h>

namespace LuaCore::Savestate {
static int SaveFileSavestate(lua_State *L) {
  const std::string path = lua_tostring(L, 1);

  ++g_vr_wait_before_input_poll;
  AsyncExecutor::invoke_async([=] {
    --g_vr_wait_before_input_poll;
    ::Savestates::do_file(path, ::Savestates::Job::Save);
  });

  return 0;
}

static int LoadFileSavestate(lua_State *L) {
  const std::string path = lua_tostring(L, 1);

  ++g_vr_wait_before_input_poll;
  AsyncExecutor::invoke_async([=] {
    --g_vr_wait_before_input_poll;
    ::Savestates::do_file(path, ::Savestates::Job::Load);
  });

  return 0;
}
} // namespace LuaCore::Savestate
