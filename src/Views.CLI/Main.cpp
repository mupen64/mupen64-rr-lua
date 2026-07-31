#include "Main.hpp"

core_cfg g_config;
core_params g_core_params {};
core_ctx* g_core = nullptr;

int main(int argc, char* argv[]) {
  g_core_params.cfg = &g_config;
}

