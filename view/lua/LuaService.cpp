#include <shared/services/LuaService.h>
#include <view/gui/Main.h>
#include <view/lua/LuaConsole.h>

namespace LuaService {
// OPTIMIZATION: If no lua scripts are running, skip the deeper lua path
#define RET_IF_EMPTY                                                           \
  {                                                                            \
    if (g_hwnd_lua_map.empty())                                                \
      return;                                                                  \
  }

t_window_procedure_params window_proc_params = {0};
int current_input_n = 0;

int pcall_no_params(lua_State *L) { return lua_pcall(L, 0, 0, 0); }

int AtInput(lua_State *L) {
  lua_pushinteger(L, current_input_n);
  return lua_pcall(L, 1, 0, 0);
}

int AtWindowMessage(lua_State *L) {
  lua_pushinteger(L, (unsigned)window_proc_params.wnd);
  lua_pushinteger(L, window_proc_params.msg);
  lua_pushinteger(L, window_proc_params.w_param);
  lua_pushinteger(L, window_proc_params.l_param);

  return lua_pcall(L, 4, 0, 0);
}

#pragma region Call Implementations
BUTTONS get_last_controller_data(int index) {
  return last_controller_data[index];
}

void call_window_message(void *wnd, unsigned int msg, unsigned int w, long l) {
  RET_IF_EMPTY;
  // Invoking dispatcher here isn't allowed, as it would lead to infinite
  // recursion
  window_proc_params = {
      .wnd = (HWND)wnd, .msg = msg, .w_param = w, .l_param = l};
  invoke_callbacks_with_key_on_all_instances(AtWindowMessage,
                                             REG_WINDOWMESSAGE);
}

void call_vi() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATVI);
  });
}

void call_input(BUTTONS *input, int index) {
  // NOTE: Special callback, we store the input data for all scripts to access
  // via joypad.get(n) If they request a change via joypad.set(n, input), we
  // change the input
  last_controller_data[index] = *input;

  RET_IF_EMPTY;

  g_main_window_dispatcher->invoke([=] {
    current_input_n = index;
    invoke_callbacks_with_key_on_all_instances(AtInput, REG_ATINPUT);
    inputCount++;
  });

  if (overwrite_controller_data[index]) {
    *input = new_controller_data[index];
    last_controller_data[index] = *input;
    overwrite_controller_data[index] = false;
  }
}

void call_interval() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATINTERVAL);
  });
}

void call_play_movie() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params,
                                               REG_ATPLAYMOVIE);
  });
}

void call_stop_movie() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params,
                                               REG_ATSTOPMOVIE);
  });
}

void call_load_state() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params,
                                               REG_ATLOADSTATE);
  });
}

void call_save_state() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params,
                                               REG_ATSAVESTATE);
  });
}

void call_reset() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATRESET);
  });
}

void call_seek_completed() {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([] {
    invoke_callbacks_with_key_on_all_instances(pcall_no_params,
                                               REG_ATSEEKCOMPLETED);
  });
}

void call_warp_modify_status_changed(const int32_t status) {
  RET_IF_EMPTY;
  g_main_window_dispatcher->invoke([=] {
    invoke_callbacks_with_key_on_all_instances(
        [=](lua_State *L) {
          lua_pushinteger(L, status);
          return lua_pcall(L, 1, 0, 0);
        },
        REG_ATWARPMODIFYSTATUSCHANGED);
  });
}
#pragma endregion
} // namespace LuaService
