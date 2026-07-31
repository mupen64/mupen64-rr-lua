#include "Main.hpp"
#include "Plugin.hpp"
#include <future>

core_cfg g_config;
core_params g_core_params{};
core_ctx *g_core = nullptr;

void clear_plugin_funcs()
{
    g_core_params.video_process_dlist = [](auto...) {};
    g_core_params.video_process_rdp_list = [](auto...) {};
    g_core_params.video_show_cfb = [](auto...) {};
    g_core_params.video_vi_status_changed = [](auto...) {};
    g_core_params.video_vi_width_changed = [](auto...) {};
    g_core_params.video_get_video_size = [](auto...) {};
    g_core_params.video_fb_read = [](auto...) {};
    g_core_params.video_fb_write = [](auto...) {};
    g_core_params.video_fb_get_frame_buffer_info = [](auto...) {};
    g_core_params.audio_ai_dacrate_changed = [](auto...) {};
    g_core_params.audio_ai_len_changed = [](auto...) {};
    g_core_params.audio_ai_read_length = [](auto...) { return 0; };
    g_core_params.audio_process_alist = [](auto...) {};
    g_core_params.input_controller_command = [](auto...) {};
    g_core_params.input_get_keys = [](auto...) {};
    g_core_params.input_set_keys = [](auto...) {};
    g_core_params.input_read_controller = [](auto...) {};
    g_core_params.rsp_do_rsp_cycles = [](auto...) { return 0; };
}

static void init_core()
{
    g_core_params.cfg = &g_config;
    clear_plugin_funcs();

    // EXTRA CALLBACKS
    // =====================================================


    // MAIN CORE CALLBACKS
    // =====================================================

    g_core_params.load_plugins = PluginUtil::load_plugins;
    g_core_params.initiate_plugins = PluginUtil::initiate_plugins;
    g_core_params.submit_task = [](const auto &cb) {
        // Defer to the stdlib's thread pool.
        (void)std::async(cb);
    };
    g_core_params.get_saves_directory = []() { return IOUtils::exe_path() / "saves"; };
    g_core_params.get_backups_directory = []() { return IOUtils::exe_path() / "backups"; };
    g_core_params.get_summercart_path = []() { return IOUtils::exe_path() / "saves/cart.vhd"; };
    g_core_params.show_multiple_choice_dialog = [](std::string_view id, const std::vector<std::string> &choices,
                                                   const char *str, const char *title, core_dialog_type type) {
        // TODO
        return 0;
    };
    g_core_params.show_ask_dialog = [](std::string_view id, const char *str, const char *title, bool warning) {
        // TODO
        return false;
    };
    g_core_params.show_dialog = [](const char *str, const char *title, core_dialog_type type) {
        // TODO
    };
    g_core_params.get_plugin_names = PluginUtil::get_plugin_names;

    core_create(&g_core_params, &g_core);
}

int main(int argc, char *argv[])
{
    init_core();
}
