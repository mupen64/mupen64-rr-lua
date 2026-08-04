#include "Plugin.hpp"
#include "Main.hpp"
#include "VersionNameHelpers.hpp"

// Tries to load a function from a library, returning nullptr if the load failed.
template <class T> static inline T try_load(decan::library &lib, const char *symbol)
{
    try
    {
        return (T)lib.get(symbol);
    }
    catch (const std::system_error &)
    {
        return nullptr;
    }
}

Plugin::Plugin(const std::filesystem::path &path) : m_lib(path), m_path(path)
{
    // metadata (required)
    auto get_metadata = (M64RRSpec::PtrGetMetadata)m_lib.get("M64RRGetMetadata");

    M64RRSpec::PluginMetadata metadata;
    get_metadata(&metadata);

    // Plugins are tied to one version of Mupen
    std::string_view target_version{metadata.target_version,
                                    strnlen(metadata.target_version, sizeof(metadata.target_version))};
    if (!target_version.empty() && target_version != CURRENT_VERSION)
        throw PluginLoadFailed("Expected target version " CURRENT_VERSION);

    std::string_view name = {metadata.name, strnlen(metadata.name, sizeof(metadata.name))};
    m_name = name;

    m_type = metadata.type;

    m_process_event = try_load<M64RRSpec::PtrProcessEvent>(m_lib, "M64RRProcessEvent");
}

void Plugin::initiate()
{
    if (!m_init_data)
    {
        m_init_data.reset(new M64RRSpec::PluginInit);
        m_init_data->rom = g_core->rom;
        m_init_data->rom = g_core->rom;

        m_init_data->rom = g_core->rom;
        m_init_data->rdram = (uint8_t *)g_core->rdram;
        m_init_data->dmem = (uint8_t *)g_core->SP_DMEM;
        m_init_data->imem = (uint8_t *)g_core->SP_IMEM;

        m_init_data->rdram_register = g_core->rdram_register;
        m_init_data->mi_register = g_core->MI_register;
        m_init_data->pi_register = g_core->pi_register;
        m_init_data->sp_register = g_core->sp_register;
        m_init_data->rsp_register = g_core->rsp_register;
        m_init_data->si_register = g_core->si_register;
        m_init_data->vi_register = g_core->vi_register;
        m_init_data->ri_register = g_core->ri_register;
        m_init_data->ai_register = g_core->ai_register;
        m_init_data->dpc_register = g_core->dpc_register;
        m_init_data->dps_register = g_core->dps_register;

        // void(CALL *process_dlist)(void);

        // Controller *controllers;
    }

    M64RRSpec::Event init_event{.initiate = {.type = M64RRSpec::Event::Type::Initiate, .init = m_init_data.get()}};

    m_process_event(init_event);

    switch (m_type)
    {
    case M64RRSpec::PluginType::Video: {
        auto process_d_list = try_load<M64RRSpec::PtrProcessDList>(m_lib, "M64RRProcessDList");

        if (process_d_list != nullptr) g_core_params.video_process_dlist = process_d_list;
    }
    break;
    case M64RRSpec::PluginType::Audio: {
        auto ai_dacrate_changed = try_load<M64RRSpec::PtrAIDacrateChanged>(m_lib, "M64RRAIDacrateChanged");
        auto ai_len_changed = try_load<M64RRSpec::PtrAILenChanged>(m_lib, "M64RRAILenChanged");

        if (ai_dacrate_changed != nullptr) g_core_params.audio_ai_dacrate_changed = ai_dacrate_changed;
        if (ai_len_changed != nullptr) g_core_params.audio_ai_len_changed = ai_len_changed;
    }
    break;
    case M64RRSpec::PluginType::Input: {
        auto get_keys = try_load<M64RRSpec::PtrGetKeys>(m_lib, "M64RRGetKeys");
        auto set_keys = try_load<M64RRSpec::PtrSetKeys>(m_lib, "M64RRSetKeys");
        auto read_controller = try_load<M64RRSpec::PtrReadController>(m_lib, "M64RRReadController");

        // TODO: update spec for GetKeys and SetKeys to avoid trampoline
        if (get_keys != nullptr)
        {
            g_core_params.input_get_keys = [=](uint32_t port, CoreButtons *buttons) {
                if (buttons == nullptr) return;
                M64RRSpec::Buttons plugin_buttons{.value = 0};
                get_keys(port, &plugin_buttons);
                buttons->value = plugin_buttons.value;
            };
        }
        if (set_keys != nullptr)
        {
            g_core_params.input_set_keys = [=](uint32_t port, CoreButtons buttons) {
                M64RRSpec::Buttons plugin_buttons{.value = buttons.value};
                set_keys(port, &plugin_buttons);
            };
        }
        if (read_controller != nullptr) g_core_params.input_read_controller = read_controller;
    }
    break;
    case M64RRSpec::PluginType::RSP: {
        auto do_rsp_cycles = try_load<M64RRSpec::PtrDoRSPCycles>(m_lib, "M64RRDoRSPCycles");

        // TODO: update spec for DoRSPCycles to avoid trampoline
        if (do_rsp_cycles != nullptr)
        {
            g_core_params.rsp_do_rsp_cycles = [=](uint32_t cycles) -> uint32_t {
                do_rsp_cycles(cycles);
                return 0;
            };
        }
    }
    break;
    }
}

struct PluginSet
{
    Plugin video;
    Plugin audio;
    Plugin input;
    Plugin rsp;
};

static std::optional<PluginSet> g_plugins = std::nullopt;
static std::mutex g_plugin_lock;

bool PluginUtil::load_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    g_plugins.emplace(Plugin(IOUtils::exe_path().parent_path() / "NoVideo.dll"),
                      Plugin(IOUtils::exe_path().parent_path() / "NoAudio.dll"),
                      Plugin(IOUtils::exe_path().parent_path() / "NoInput.dll"),
                      Plugin(IOUtils::exe_path().parent_path() / "TASRSP.dll"));
    return false;
}
void PluginUtil::initiate_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    clear_plugin_funcs();

    if (!g_plugins.has_value()) abort();

    g_plugins->video.initiate();
    g_plugins->audio.initiate();
    g_plugins->input.initiate();
    g_plugins->rsp.initiate();
}
void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    std::scoped_lock lock(g_plugin_lock);
    if (video) strncpy_s(video, 64, g_plugins->video.name().c_str(), 64);
    if (audio) strncpy_s(audio, 64, g_plugins->audio.name().c_str(), 64);
    if (input) strncpy_s(input, 64, g_plugins->input.name().c_str(), 64);
    if (rsp) strncpy_s(rsp, 64, g_plugins->rsp.name().c_str(), 64);
}