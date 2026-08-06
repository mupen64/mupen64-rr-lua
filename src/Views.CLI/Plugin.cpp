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

template <class T>
static inline void try_load_function(decan::library &lib, const char *symbol,
                                     std::function<std::remove_pointer_t<T>> &func)
{
    auto pointer = try_load<T>(lib, symbol);
    if (pointer != nullptr) func = pointer;
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

        if (g_core_params.video_process_dlist)
        {
            // We know that the std::function should contain a function pointer, so this shouldn't fail
            m_init_data->process_dlist = *g_core_params.video_process_dlist.target<M64RRSpec::PtrProcessDList>();
        }

        m_init_data->controllers = g_core_params.controls;
    }

    M64RRSpec::Event init_event{.initiate = {.type = M64RRSpec::Event::Type::Initiate, .init = m_init_data.get()}};

    m_process_event(init_event);
}

void Plugin::bind_functions()
{
    switch (m_type)
    {
    case M64RRSpec::PluginType::Video:
        try_load_function<M64RRSpec::PtrProcessDList>(m_lib, "M64RRProcessDList", g_core_params.video_process_dlist);
        break;
    case M64RRSpec::PluginType::Audio:
        try_load_function<M64RRSpec::PtrAIDacrateChanged>(m_lib, "M64RRAIDacrateChanged",
                                                          g_core_params.audio_ai_dacrate_changed);
        try_load_function<M64RRSpec::PtrAILenChanged>(m_lib, "M64RRAILenChanged", g_core_params.audio_ai_len_changed);
        break;
    case M64RRSpec::PluginType::Input:
        try_load_function<M64RRSpec::PtrGetKeys>(m_lib, "M64RRGetKeys", g_core_params.input_get_keys);
        try_load_function<M64RRSpec::PtrSetKeys>(m_lib, "M64RRSetKeys", g_core_params.input_set_keys);
        try_load_function<M64RRSpec::PtrReadController>(m_lib, "M64RRReadController",
                                                        g_core_params.input_read_controller);
        break;
    case M64RRSpec::PluginType::RSP:
        try_load_function<M64RRSpec::PtrDoRSPCycles>(m_lib, "M64RRDoRSPCycles", g_core_params.rsp_do_rsp_cycles);
        break;
    }
}

void Plugin::send_event(M64RRSpec::Event event)
{
    m_process_event(event);
}

bool PluginUtil::load_plugins()
{
    std::scoped_lock lock(g_plugin_lock);
    g_plugins.emplace(Plugin(IOUtils::exe_path().parent_path() / "plugins/NoVideo.dll"),
                      Plugin(IOUtils::exe_path().parent_path() / "plugins/NoAudio.dll"),
                      Plugin(IOUtils::exe_path().parent_path() / "plugins/NoInput.dll"),
                      Plugin(IOUtils::exe_path().parent_path() / "plugins/TASRSP.dll"));
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
void PluginUtil::start_plugins()
{

    g_plugins->video.bind_functions();
    g_plugins->audio.bind_functions();
    g_plugins->input.bind_functions();
    g_plugins->rsp.bind_functions();
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomOpened});
}
void PluginUtil::stop_plugins()
{
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::RomClosed});
    send_event(M64RRSpec::Event{.type = M64RRSpec::Event::Type::Shutdown});
}
void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    std::scoped_lock lock(g_plugin_lock);
    if (video) strncpy_s(video, 64, g_plugins->video.name().c_str(), 64);
    if (audio) strncpy_s(audio, 64, g_plugins->audio.name().c_str(), 64);
    if (input) strncpy_s(input, 64, g_plugins->input.name().c_str(), 64);
    if (rsp) strncpy_s(rsp, 64, g_plugins->rsp.name().c_str(), 64);
}
void PluginUtil::send_event(M64RRSpec::Event event)
{
    std::scoped_lock lock(g_plugin_lock);

    if (!g_plugins.has_value()) abort();

    g_plugins->video.send_event(event);
    g_plugins->audio.send_event(event);
    g_plugins->input.send_event(event);
    g_plugins->rsp.send_event(event);
}