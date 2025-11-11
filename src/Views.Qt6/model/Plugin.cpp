#include "Plugin.hpp"
#include "core_plugin.h"
#include "mupapi.h"

#define MUP_EXTRACT_FN(lib, name) ((fp_##name)lib.resolve(#name))

#pragma region Dummy Functions

static uint32_t CALL dummy_do_rsp_cycles(uint32_t Cycles)
{
    return Cycles;
}

static void CALL dummy_void()
{
}

static int32_t CALL dummy_initiate_gfx(core_gfx_info)
{
    return 1;
}

static int32_t CALL dummy_initiate_audio(core_audio_info)
{
    return 1;
}

static void CALL dummy_initiate_controllers(core_input_info)
{
}

static void CALL dummy_ai_dacrate_changed(int32_t)
{
}

static uint32_t CALL dummy_ai_read_length()
{
    return 0;
}

static void CALL dummy_ai_update(int32_t)
{
}

static void CALL dummy_controller_command(int32_t, uint8_t *)
{
}

static void CALL dummy_get_keys(int32_t, core_buttons *)
{
}

static void CALL dummy_set_keys(int32_t, core_buttons)
{
}

static void CALL dummy_read_controller(int32_t, uint8_t *)
{
}

static void CALL dummy_key_down(uint32_t, int32_t)
{
}

static void CALL dummy_key_up(uint32_t, int32_t)
{
}

static void CALL dummy_initiate_rsp(core_rsp_info, uint32_t *)
{
}

static void CALL dummy_fb_read(uint32_t)
{
}

static void CALL dummy_fb_write(uint32_t, uint32_t)
{
}

static void CALL dummy_fb_get_framebuffer_info(void *)
{
}

static void CALL dummy_move_screen(int32_t, int32_t)
{
}

#pragma endregion

namespace Mupen {
    PluginInfo extract_plugin_info(std::filesystem::path&& path) {
        
    }
}