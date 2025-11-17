#include "Plugin.hpp"
#include "Core.hpp"
#include "core_api.h"
#include "core_plugin.h"
#include "mupapi.h"
#include <boost/dll.hpp>
#include <boost/filesystem/detail/path_traits.hpp>
#include <stdexcept>

namespace dll = boost::dll;
namespace bfs = boost::filesystem;

#define MUP_FN(name) get<fp_##name>(#name)

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

namespace Mupen
{
PluginInfo extract_plugin_info(const std::filesystem::path &path)
{

    core_plugin_info info{};

    // Load the plugin just to call the get_info function.
    {
        auto plugin_lib = dll::shared_library(bfs::path(path));
        auto mup_get_info = plugin_lib.get<fp_mup_get_info>("mup_get_info");
        mup_get_info(&info);
    }

    return {.path = path, .info = std::move(info)};
}

PluginSet::PluginSet(mup_core_functions core_functions, std::filesystem::path video_path, std::filesystem::path audio_path,
                     std::filesystem::path input_path, std::filesystem::path rsp_path)
    : m_video_plugin(bfs::path(video_path)), m_audio_plugin(bfs::path(audio_path)),
      m_input_plugin(bfs::path(input_path)), m_rsp_plugin(bfs::path(rsp_path))
{
    {
        // check that all 4 plugins are the correct type
        core_plugin_info info = {};

        m_video_plugin.MUP_FN(mup_get_info)(&info);
        if (info.type != plugin_video) throw std::invalid_argument("video plugin path does not point to a video plugin");

        info = {};
        m_audio_plugin.MUP_FN(mup_get_info)(&info);
        if (info.type != plugin_audio) throw std::invalid_argument("audio plugin path does not point to an audio plugin");

        info = {};
        m_input_plugin.MUP_FN(mup_get_info)(&info);
        if (info.type != plugin_input) throw std::invalid_argument("input plugin path does not point to an input plugin");

        info = {};
        m_rsp_plugin.MUP_FN(mup_get_info)(&info);
        if (info.type != plugin_rsp) throw std::invalid_argument("RSP plugin path does not point to an RSP plugin");

        auto exe_path_str = boost::dll::program_location().string();

        m_video_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &core_functions);
        m_audio_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &core_functions);
        m_input_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &core_functions);
        m_rsp_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &core_functions);
    }
}

PluginSet::~PluginSet() {
    
}

void PluginSet::resolve_functions_to(core_params &params)
{
    // video plugin
    params.video_process_dlist = m_video_plugin.MUP_FN(mupv_process_d_list);
    params.video_process_rdp_list = m_video_plugin.MUP_FN(mupv_process_rdp_list);
    params.video_show_cfb = m_video_plugin.MUP_FN(mupv_show_cfb);
    params.video_vi_status_changed = m_video_plugin.MUP_FN(mupv_vi_status_changed);
    params.video_vi_width_changed = m_video_plugin.MUP_FN(mupv_vi_width_changed);
    params.video_fb_read = m_video_plugin.MUP_FN(mupv_fb_read);
    params.video_fb_write = m_video_plugin.MUP_FN(mupv_fb_write);
    params.video_fb_get_frame_buffer_info = m_video_plugin.MUP_FN(mupv_get_fb_info);

    // audio plugin
    params.audio_ai_dacrate_changed = m_audio_plugin.MUP_FN(mupa_ai_dacrate_changed);
    params.audio_ai_len_changed = m_audio_plugin.MUP_FN(mupa_ai_len_changed);
    params.audio_ai_read_length = m_audio_plugin.MUP_FN(mupa_ai_read_length);
    params.audio_process_alist = m_audio_plugin.MUP_FN(mupa_process_a_list);
    params.audio_ai_update = m_audio_plugin.MUP_FN(mupa_ai_update);

    // input plugin
    params.input_controller_command = m_input_plugin.MUP_FN(mupi_controller_command);
    params.input_get_keys = m_input_plugin.MUP_FN(mupi_get_keys);
    params.input_set_keys = m_input_plugin.MUP_FN(mupi_set_keys);
    params.input_read_controller = m_input_plugin.MUP_FN(mupi_read_controller);

    // rsp plugin
    params.rsp_do_rsp_cycles = m_rsp_plugin.MUP_FN(mupr_do_rsp_cycles);
}

void PluginSet::initiate_video(core_ctx &ctx,
                               const ICoreService& core_service)
{
    auto gfx_info = core_gfx_info{
        .byteswapped = 1,
        .rom = ctx.rom,
        .rdram = (uint8_t *)ctx.rdram,
        .dmem = (uint8_t *)ctx.SP_DMEM,
        .imem = (uint8_t *)ctx.SP_IMEM,
        .mi_intr_reg = &ctx.MI_register->mi_intr_reg,
        .dpc_start_reg = &ctx.dpc_register->dpc_start,
        .dpc_end_reg = &ctx.dpc_register->dpc_end,
        .dpc_current_reg = &ctx.dpc_register->dpc_current,
        .dpc_status_reg = &ctx.dpc_register->dpc_status,
        .dpc_clock_reg = &ctx.dpc_register->dpc_clock,
        .dpc_bufbusy_reg = &ctx.dpc_register->dpc_bufbusy,
        .dpc_pipebusy_reg = &ctx.dpc_register->dpc_pipebusy,
        .dpc_tmem_reg = &ctx.dpc_register->dpc_tmem,
        .vi_status_reg = &ctx.vi_register->vi_status,
        .vi_origin_reg = &ctx.vi_register->vi_origin,
        .vi_width_reg = &ctx.vi_register->vi_width,
        .vi_intr_reg = &ctx.vi_register->vi_v_intr,
        .vi_v_current_line_reg = &ctx.vi_register->vi_current,
        .vi_timing_reg = &ctx.vi_register->vi_burst,
        .vi_v_sync_reg = &ctx.vi_register->vi_v_sync,
        .vi_h_sync_reg = &ctx.vi_register->vi_h_sync,
        .vi_leap_reg = &ctx.vi_register->vi_leap,
        .vi_h_start_reg = &ctx.vi_register->vi_h_start,
        .vi_v_start_reg = &ctx.vi_register->vi_v_start,
        .vi_v_burst_reg = &ctx.vi_register->vi_v_burst,
        .vi_x_scale_reg = &ctx.vi_register->vi_x_scale,
        .vi_y_scale_reg = &ctx.vi_register->vi_y_scale,
    };
    // init and request window settings
    auto wm_settings = mupv_wm_settings{};
    m_video_plugin.MUP_FN(mupv_init)(gfx_info, &wm_settings);

    // pass child window to gfx
    auto wm_handle = core_service.setup_window(wm_settings);
    m_video_plugin.MUP_FN(mupv_receive_child_window)(wm_handle);
}

void PluginSet::initiate_audio(core_ctx &ctx)
{
    auto audio_info = core_audio_info{
        .byteswapped = 1,
        .rom = ctx.rom,
        .dmem = (uint8_t *)ctx.SP_DMEM,
        .imem = (uint8_t *)ctx.SP_IMEM,
        .mi_intr_reg = &ctx.MI_register->mi_intr_reg,
        .ai_dram_addr_reg = &ctx.ai_register->ai_dram_addr,
        .ai_len_reg = &ctx.ai_register->ai_len,
        .ai_control_reg = &ctx.ai_register->ai_control,
        .ai_status_reg = &ctx.ai_register->ai_status,
        .ai_dacrate_reg = &ctx.ai_register->ai_dacrate,
        .ai_bitrate_reg = &ctx.ai_register->ai_bitrate,
        .check_interrupts = dummy_void,
    };
    m_audio_plugin.MUP_FN(mupa_init)(audio_info);
}

void PluginSet::initiate_input(core_ctx &ctx, core_params &params)
{
    auto input_info = core_input_info{
        .byteswapped = 1,
        .header = ctx.rom,
        .controllers = params.controls,
    };
    m_input_plugin.MUP_FN(mupi_init)(input_info);
}

void PluginSet::initiate_rsp(core_ctx &ctx, core_params& params)
{
    auto rsp_info = core_rsp_info{
        .byteswapped = 1,
        .rdram = (uint8_t *)ctx.rdram,
        .dmem = (uint8_t *)ctx.SP_DMEM,
        .imem = (uint8_t *)ctx.SP_IMEM,
        .mi_intr_reg = &ctx.MI_register->mi_intr_reg,
        .sp_mem_addr_reg = &ctx.sp_register->sp_mem_addr_reg,
        .sp_dram_addr_reg = &ctx.sp_register->sp_dram_addr_reg,
        .sp_rd_len_reg = &ctx.sp_register->sp_rd_len_reg,
        .sp_wr_len_reg = &ctx.sp_register->sp_wr_len_reg,
        .sp_status_reg = &ctx.sp_register->sp_status_reg,
        .sp_dma_full_reg = &ctx.sp_register->sp_dma_full_reg,
        .sp_dma_busy_reg = &ctx.sp_register->sp_dma_busy_reg,
        .sp_pc_reg = &ctx.rsp_register->rsp_pc,
        .sp_semaphore_reg = &ctx.sp_register->sp_semaphore_reg,
        .dpc_start_reg = &ctx.dpc_register->dpc_start,
        .dpc_end_reg = &ctx.dpc_register->dpc_end,
        .dpc_current_reg = &ctx.dpc_register->dpc_current,
        .dpc_status_reg = &ctx.dpc_register->dpc_status,
        .dpc_clock_reg = &ctx.dpc_register->dpc_clock,
        .dpc_bufbusy_reg = &ctx.dpc_register->dpc_bufbusy,
        .dpc_pipebusy_reg = &ctx.dpc_register->dpc_pipebusy,
        .dpc_tmem_reg = &ctx.dpc_register->dpc_tmem,
        .check_interrupts = dummy_void,
        .process_dlist_list = params.video_process_dlist,
        .process_alist_list = params.audio_process_alist,
        .process_rdp_list = params.video_process_rdp_list,
        .show_cfb = params.video_show_cfb,
    };
    m_rsp_plugin.MUP_FN(mupr_init)(rsp_info);
}

void PluginSet::initiate_all(core_ctx &ctx, core_params &params, const ICoreService& core_service)
{
    initiate_video(ctx, core_service);
    initiate_audio(ctx);
    initiate_input(ctx, params);
    initiate_rsp(ctx, params);
}
} // namespace Mupen