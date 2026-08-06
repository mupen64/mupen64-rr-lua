#pragma once
#include <decan.hpp>
#include <m64rr/API.hpp>
#include <m64rr/Plugin.hpp>

class PluginLoadFailed : std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

class Plugin
{
  public:
    Plugin(const std::filesystem::path &path);

    /**
     * @brief Triggers the `Initiate` event and sets up necessary initialization data.
     */
    void initiate();

    /**
     * @brief Binds the needed functions from this plugin to the core.
     */
    void bind_functions();

    /**
     * @brief Triggers an arbitrary lifecycle event.
     * 
     * @param event The event
     */
    void send_event(M64RRSpec::Event event);

    /**
     * \brief Gets the plugin's path
     */
    auto path() const { return m_path; }

    /**
     * \brief Gets the plugin's name
     */
    auto name() const { return m_name; }

    /**
     * \brief Gets the plugin's type
     */
    auto type() const { return m_type; }

  private:
    decan::library m_lib;

    std::filesystem::path m_path;
    std::string m_name;
    M64RRSpec::PluginType m_type;

    M64RRSpec::PtrProcessEvent m_process_event;

    std::unique_ptr<M64RRSpec::PluginInit> m_init_data;
};

namespace PluginUtil
{
bool load_plugins();
void initiate_plugins();
void start_plugins();
void stop_plugins();
void get_plugin_names(char *video, char *audio, char *input, char *rsp);
void send_event(M64RRSpec::Event event);
} // namespace PluginUtil