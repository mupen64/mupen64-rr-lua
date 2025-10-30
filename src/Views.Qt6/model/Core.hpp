/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MODEL_CORE_HPP_INCLUDED
#define MODEL_CORE_HPP_INCLUDED

#include <core_api.h>

namespace Mupen
{
class Core
{
  public:
    static void init(core_cfg config);
    static Core& instance();
    static void shutdown();

    core_ctx *operator->() { return m_ctx; }

  private:
    Core(core_cfg config);
    ~Core();

    core_params m_params;
    core_ctx *m_ctx;
};
} // namespace Mupen

#endif