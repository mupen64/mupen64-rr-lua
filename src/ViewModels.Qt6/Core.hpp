#pragma once

#include <m64rr/API.hpp>
namespace Core 
{
core_cfg& config();
core_params& params();
core_ctx* context();

void clear_plugin_funcs(core_params& params);
}