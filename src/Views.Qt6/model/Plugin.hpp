#ifndef MODEL_PLUGIN_HPP_INCLUDED
#define MODEL_PLUGIN_HPP_INCLUDED

#include "core_api.h"
#include "core_plugin.h"
#include "mupapi.h"
#include <boost/dll/shared_library.hpp>
#include <filesystem>
#include <utility>

namespace Mupen
{
struct PluginInfo
{
    std::filesystem::path path;
    core_plugin_info info;
};

PluginInfo extract_plugin_info(std::filesystem::path path);

class PluginSet final
{
  public:
    PluginSet() = delete;
    PluginSet(const PluginSet &) = delete;
    PluginSet(PluginSet &&src)
        : m_video_plugin(std::move(src.m_video_plugin)), m_audio_plugin(std::move(src.m_audio_plugin)),
          m_input_plugin(std::move(src.m_input_plugin)), m_rsp_plugin(std::move(src.m_rsp_plugin))
    {
    }

    PluginSet &operator=(const PluginSet &) = delete;
    PluginSet &operator=(PluginSet &&src)
    {
        m_video_plugin = std::move(src.m_video_plugin);
        m_audio_plugin = std::move(src.m_audio_plugin);
        m_input_plugin = std::move(src.m_input_plugin);
        m_rsp_plugin = std::move(src.m_rsp_plugin);
        return *this;
    }

    PluginSet(std::filesystem::path video_path, std::filesystem::path audio_path, std::filesystem::path input_path,
              std::filesystem::path rsp_path);

    ~PluginSet();

    void resolve_functions_to(core_params &params);
    void initiate_all(core_ctx &ctx, std::function<mup_wm_handle(const mupv_wm_settings&)> create_window);

  private:
    boost::dll::shared_library m_video_plugin;
    boost::dll::shared_library m_audio_plugin;
    boost::dll::shared_library m_input_plugin;
    boost::dll::shared_library m_rsp_plugin;
};
} // namespace Mupen

#endif