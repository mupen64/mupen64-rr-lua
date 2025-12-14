/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MODEL_CORE_HPP_INCLUDED
#define MODEL_CORE_HPP_INCLUDED

#include <core_api.h>
#include <memory>
#include <span>
#include <spdlog/common.h>
#include <string>
#include <string_view>

#include "Plugin.hpp"
#include "core_types.h"
#include "mupapi.h"
#include "StrUtils.h"
namespace Mupen
{
extern core_cfg g_core_cfg;
extern core_params g_core_params;
extern core_ctx *g_core_ctx;

class IWindowService
{
  public:
    IWindowService() {}
    virtual ~IWindowService() {}

    virtual core_result open_window(uint32_t width, uint32_t height) = 0;
    virtual core_result close_window() = 0;
};

class IOpenGLService : public IWindowService
{
  public:
    IOpenGLService() : IWindowService() {}

    virtual core_result request_attrs(const mupv_gl_buffer_attr *attrs, const int32_t *vals, size_t len) = 0;
    virtual core_result request_version(mupv_gl_profile profile, uint32_t major, uint32_t minor) = 0;

    virtual core_result query_attrs(const mupv_gl_buffer_attr *attrs, int32_t *vals, size_t len) = 0;
    virtual core_result query_version(mupv_gl_profile *profile, uint32_t *major, uint32_t *minor) = 0;
    virtual core_result query_default_fbo(uint32_t *fbo) = 0;

    virtual core_result swap_buffers() = 0;
};

/**
 * @brief Interface for core-needed functions from the view.
 *
 * Implementors should delegate any data storage to a central
 * source, such that it may be freely copied as needed.
 */
class ICoreService
{
  public:
    ICoreService() {}

    virtual ~ICoreService() {}

    /**
     * @brief Display a multiple-choice dialog, if the user has not requested to hide it.
     *
     * @param id An ID string.
     * @param choices The choices to select between.
     * @param title The title of the dialog.
     * @param message The message of the dialog.
     * @param type The icon to display alongside the dialog text.
     * @return Index of the chosen option; SIZE_MAX if cancelled. If the user has requested
     * not to see this dialog, returns the last chosen option.
     */
    virtual size_t show_choice_dialog(std::string_view id, std::span<const std::string> choices, std::string_view title,
                                      std::string_view message, core_dialog_type type) = 0;

    /**
     * @brief Display an info dialog with an "OK" button.
     *
     * @param title The title of the dialog.
     * @param message The message of the dialog.
     * @param type The icon to display alongside the dialog text.
     */
    virtual void show_info_dialog(std::string_view title, std::string_view message, core_dialog_type type) = 0;

    /**
     * @brief Create a window service for the specified graphics API.
     * 
     * @return IWindowService An IWindowService for the corresponding API.
     */
    virtual std::unique_ptr<IWindowService> init_window_service(mupv_graphics_api api) = 0;
};

void core_init(core_cfg config, std::unique_ptr<ICoreService> &&core_service);

void core_start(const std::filesystem::path &rom_path, const PluginPaths &plugin_paths);

void core_stop();
} // namespace Mupen

#endif