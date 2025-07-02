/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace ResizeAnchor
{
    /**
     * \brief Flags used to specify a window's anchor behaviour.
     * \details The flags can be combined using the bitwise OR operator to anchor a window on multiple sides, thereby causing the anchors to "pull" the window apart and growing it in the specified directions.
     */
    enum class AnchorFlags : uint64_t {
        /**
         * \brief Keep the window's X position relative to the parent window constant. This is a no-op on its own.
         */
        Left = 1 << 1,
        /**
         * \brief Maintain the right edge of the window at a constant distance from the right edge of the parent window.
         */
        Right = 1 << 2,
        /**
         * \brief Keep the window's Y position relative to the parent window constant. This is a no-op on its own.
         */
        Top = 1 << 3,
        /**
         * \brief Maintain the bottom edge of the window at a constant distance from the bottom edge of the parent window.
         */
        Bottom = 1 << 4,
        /**
         * \brief Invalidates the window when resized.
         */
        Invalidate = 1 << 5,
        /**
         * \brief Erases the window graphics when resized.
         */
        Erase = 1 << 6,
    };

    constexpr AnchorFlags operator|(AnchorFlags a, AnchorFlags b)
    {
        return static_cast<AnchorFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    constexpr AnchorFlags operator&(AnchorFlags a, AnchorFlags b)
    {
        return static_cast<AnchorFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
    }

    constexpr AnchorFlags FULL_ANCHOR = AnchorFlags::Left | AnchorFlags::Right | AnchorFlags::Top | AnchorFlags::Bottom;
    constexpr AnchorFlags HORIZONTAL_ANCHOR = AnchorFlags::Left | AnchorFlags::Right;
    constexpr AnchorFlags VERTICAL_ANCHOR = AnchorFlags::Top | AnchorFlags::Bottom;
    constexpr AnchorFlags INVALIDATE_ERASE = AnchorFlags::Invalidate | AnchorFlags::Erase;

    /**
     * \brief Adds anchors to a window.
     * \param hwnd The handle of the window to which anchors will be added.
     * \param anchors A vector of pairs where each pair contains a child window handle and its associated anchor flags.
     * \param replace_child_anchors If true, existing anchors for child windows will be replaced with the new ones. If false, only new anchors will be added.
     * \return Whether the operation succeeded.
     * \details If the window already has anchors, they will be updated based on the <c>replace_child_anchors</c> parameter.
     * \remarks This function must be called before any resizing of the parent window occurs. A good place to call this is during WM_INITDIALOG (dialog) or WM_CREATE (window) processing.
     * \remarks The window handles provided in `anchors` must be direct children of the window provided via `hwnd`.
     * \remarks The resizing hook will be removed when the window is destroyed.
     */
    bool add_anchors(HWND hwnd, const std::vector<std::pair<HWND, AnchorFlags>>& anchors, bool replace_child_anchors = true);

    /**
     * \brief Removes an anchor from a child window.
     * \param hwnd The handle of the parent window from which the anchor will be removed.
     * \param child_hwnd The handle of the child window whose anchor will be removed.
     * \return Whether the operation succeeded.
     */
    bool remove_anchor(HWND hwnd, HWND child_hwnd);
} // namespace ResizeAnchor
