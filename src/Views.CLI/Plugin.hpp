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

    void initiate(core_params &core);

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
};

namespace PluginUtil
{
bool load_plugins();
void initiate_plugins();
void get_plugin_names(char *video, char *audio, char *input, char *rsp);
} // namespace PluginUtil