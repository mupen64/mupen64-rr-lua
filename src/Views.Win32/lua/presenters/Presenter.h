/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * A presenter is responsible for the following:
 *
 *	- Holding a Direct2D render target and exposing it to its Lua instance via dc()
 *	- Drawing the Direct2D render target graphics to a control provided externally
 *	- Blitting its graphics to arbitrary DCs
 *
 *	The init() method must be called and have succeeded prior to calling other functions.
 *
 *	This class is not thread-safe.
 */
class Presenter
{
  public:
    /**
     * Destroys the presenter and cleans up its resources
     */
    virtual ~Presenter() = default;

    /**
     * \brief Initializes the presenter
     * \param hwnd The window associated with the presenter. The window must be a child window and have the
     * WS_EX_LAYERED style. \return Whether the operation succeeded
     */

    virtual bool init(HWND hwnd) = 0;

    /**
     * \brief Creates a new Direct2D render target and adds it to the presenter's pool, returning a pointer to it. The
     * render targets are drawn in the order they were added to the pool, and are retained until the presenter is
     * destroyed or remove_dc is called.
     * \return A pointer to the new render target. The presenter retains ownership of the render target.
     */
    virtual ID2D1RenderTarget *add_rt() = 0;

    /**
     * \brief Removes a Direct2D render target from the presenter's pool. The render target is identified by its
     * pointer.
     * \param dc The render target to remove.
     */
    virtual void remove_rt(const ID2D1RenderTarget *rt) = 0;

    /**
     * \brief Resizes the presenter and its render targets. Render target handles remain valid.
     * \param size The new size.
     */
    virtual void resize(D2D1_SIZE_U size) = 0;

    /**
     * Gets the backing texture size
     */
    virtual D2D1_SIZE_U size() = 0;

    /**
     * \brief Adjusts the provided render target clear color to fit the presenter's clear color
     * \param color The color to adjust
     * \return The nearest clear color which fits the presenter
     */
    virtual D2D1::ColorF adjust_clear_color(const D2D1::ColorF color) const { return color; }

    /**
     * Presents the composited render targets.
     */
    virtual void present() = 0;

    /**
     * \brief Blits the presenter's contents to a DC at the specified position
     * \param hdc The target DC
     * \param rect The target bounds
     * \remark Fully transparent pixels in the presenter buffer will remain unchanged in the target DC
     */
    virtual void blit(HDC hdc, RECT rect) = 0;
};
