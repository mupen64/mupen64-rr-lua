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

class ICoreService;

struct PluginPaths
{
    std::filesystem::path video_path;
    std::filesystem::path audio_path;
    std::filesystem::path input_path;
    std::filesystem::path rsp_path;
};

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

    /**
     * @brief Creates a new plugin set using the 4 plugins.
     *
     * @param core_funcs A series of functions forwarded by the core.
     * @param video_path path to the video plugin
     * @param audio_path path to the audio plugin
     * @param input_path path to the input plugin
     * @param rsp_path path to the rsp plugin
     */
    PluginSet(core_plugin_extended_funcs core_funcs, std::filesystem::path video_path, std::filesystem::path audio_path,
              std::filesystem::path input_path, std::filesystem::path rsp_path);

    ~PluginSet();

    /**
     * @brief Resolves all needed functions to a core_params struct from the 4 plugins.
     *
     * @param params The core_params struct to save the function pointers to
     */
    void resolve_functions_to(core_params &params);

    /**
     * @brief Copies the names of the 4 plugins into the 4 provided pointers.
     *
     * The pointers, if non-null, should point to a slice of at least 64 bytes in length.
     * Names obtained will be trimmed to this length. If null, no copy is performed.
     * 
     * @param video Pointer to store the video plugin's name to.
     * @param audio Pointer to store the audio plugin's name to.
     * @param input Pointer to store the input plugin's name to.
     * @param rsp Pointer to store the RSP plugin's name to.
     */
    void extract_names(char* video, char* audio, char* input, char* rsp);

    /**
     * @brief Performs plugin-specific init for all 4 plugins.
     *
     * @param ctx A core_ctx created using core_create.
     * @param create_window A callback to the frontend to create a window using the provided settings.
     */
    void initiate_all(core_ctx &ctx, core_params &params, ICoreService& core_service);

  private:
    void initiate_video(core_ctx &ctx, ICoreService& core_service);
    void initiate_audio(core_ctx &ctx);
    void initiate_input(core_ctx &ctx, core_params &params);
    void initiate_rsp(core_ctx &ctx, core_params &params);

    boost::dll::shared_library m_video_plugin;
    boost::dll::shared_library m_audio_plugin;
    boost::dll::shared_library m_input_plugin;
    boost::dll::shared_library m_rsp_plugin;

    fp_mup_get_info m_video_get_info = nullptr;
    fp_mup_get_info m_audio_get_info = nullptr;
    fp_mup_get_info m_input_get_info = nullptr;
    fp_mup_get_info m_rsp_get_info = nullptr;
};
} // namespace Mupen

#endif