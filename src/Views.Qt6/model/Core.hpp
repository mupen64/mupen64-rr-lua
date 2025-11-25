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
     * @brief Assuming the UI is switched into game view, requests that the render window be created and sized.
     * 
     * @param settings The settings to apply.
     * @return mup_wm_handle A handle to the set-up window.
     */
    virtual mup_wm_handle setup_window(const mupv_wm_settings &settings) = 0;
};

void core_init(core_cfg config, std::unique_ptr<ICoreService> &&core_service);

void core_start(const std::filesystem::path &rom_path, const PluginPaths &plugin_paths);

void core_stop();
} // namespace Mupen

#endif