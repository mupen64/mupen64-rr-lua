/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for adding drag-and-drop reordering functionality to list views.
 */
namespace ReorderableListView
{

/**
 * \brief The parameters for a reorderable ListView.
 */
struct Params
{
    /**
     * \brief A callback that is called when an item is reordered.
     * \param from The original index of the item.
     * \param to The new index of the item.
     */
    std::function<void(size_t from, size_t to)> on_reorder;
};

/**
 * \brief Makes a ListView reorderable.
 * \param hwnd The ListView's handle.
 * \param parent_hwnd The ListView's parent window handle.
 * \param params The parameters.
 * \remarks The ListView must have the LVS_REPORT style. This operation can't be undone.
 */
void make_reorderable(HWND hwnd, HWND parent_hwnd, const Params &params);

} // namespace ReorderableListView
