/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <components/FilePicker.h>
#include <components/LuaDialog.h>
#include <lua/LuaManager.h>

// wParam: either nullptr, or a pointer to a t_instance_context whose running state has changed
#define MUPM_RUNNING_STATE_CHANGED (WM_USER + 24)

#define MUPM_REBUILD_INSTANCE_LIST (WM_USER + 25)

struct t_instance_context {
    HWND hwnd{};
    std::filesystem::path typed_path{};
    std::wstring logs{};
    t_lua_environment* env{};
};

struct t_dialog_state {
    HWND mgr_hwnd{};
    HWND inst_hwnd{};
    RECT initial_rect{};
    std::vector<std::shared_ptr<t_instance_context>> stored_contexts{};
};

static t_dialog_state g_dlg{};
static std::vector<std::shared_ptr<t_instance_context>> g_lua_instance_wnd_ctxs{};

static void print(t_instance_context& ctx, const std::wstring& text)
{
    constexpr auto max_buffer = 0x7000;

    if (IsWindow(ctx.hwnd))
    {
        HWND con_wnd = GetDlgItem(ctx.hwnd, IDC_LOG);
        int length = GetWindowTextLength(con_wnd);
        if (length >= max_buffer)
        {
            SendMessage(con_wnd, EM_SETSEL, 0, length / 2);
            SendMessage(con_wnd, EM_REPLACESEL, false, (LPARAM) "");
            length = GetWindowTextLength(con_wnd);
        }
        SendMessage(con_wnd, EM_SETSEL, length, length);
        SendMessage(con_wnd, EM_REPLACESEL, false, (LPARAM)text.c_str());
    }

    ctx.logs += text;

    if (ctx.logs.size() > max_buffer)
    {
        ctx.logs.erase(0, ctx.logs.size() - max_buffer);
    }
}

static void stop(t_instance_context& ctx)
{
    if (!ctx.env)
    {
        return;
    }

    LuaManager::destroy_environment(ctx.env);
    ctx.env = nullptr;
}

static void start(t_instance_context& ctx, const std::filesystem::path& path)
{
    SendMessage(ctx.hwnd, WM_COMMAND, MAKEWPARAM(IDC_STOP, BN_CLICKED), 0);

    const auto result = LuaManager::create_environment(
    path,
    [&] {
        PostMessage(ctx.hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
        PostMessage(g_dlg.mgr_hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);
    },
    [&](const std::wstring& text) {
        print(ctx, text);
    });

    if (!result.has_value())
    {
        print(ctx, result.error());
        return;
    }

    ctx.env = result.value();

    PostMessage(ctx.hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
    PostMessage(g_dlg.mgr_hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);
}

static std::shared_ptr<t_instance_context> add_and_select_instance(const std::filesystem::path& path)
{
    const auto ctx = std::make_shared<t_instance_context>();
    ctx->typed_path = path;

    g_lua_instance_wnd_ctxs.insert(g_lua_instance_wnd_ctxs.begin(), ctx);

    if (!IsWindow(g_dlg.mgr_hwnd))
    {
        return ctx;
    }

    SendMessage(g_dlg.mgr_hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);
    ListBox_SetCurSel(GetDlgItem(g_dlg.mgr_hwnd, IDC_INSTANCES), 0);
    SendMessage(g_dlg.mgr_hwnd, WM_COMMAND, MAKEWPARAM(IDC_INSTANCES, LBN_SELCHANGE), 0);

    return ctx;
}

INT_PTR CALLBACK lua_instance_dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto ctx = (t_instance_context*)GetWindowLongPtr(hwnd, GWL_USERDATA);

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, GWL_USERDATA, lparam);

        ctx = (t_instance_context*)GetWindowLongPtr(hwnd, GWL_USERDATA);

        ctx->hwnd = hwnd;

        Edit_SetText(GetDlgItem(hwnd, IDC_PATH), ctx->env ? ctx->env->path.c_str() : ctx->typed_path.c_str());
        Edit_SetText(GetDlgItem(hwnd, IDC_LOG), ctx->logs.c_str());

        PostMessage(hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);

        ResizeAnchor::add_anchors(hwnd, {
                                        {GetDlgItem(hwnd, IDC_PATH), ResizeAnchor::AnchorFlags::Left | ResizeAnchor::AnchorFlags::Right},
                                        {GetDlgItem(hwnd, IDC_BROWSE), ResizeAnchor::AnchorFlags::Right},
                                        {GetDlgItem(hwnd, IDC_LOG), ResizeAnchor::AnchorFlags::Left | ResizeAnchor::AnchorFlags::Right | ResizeAnchor::AnchorFlags::Top | ResizeAnchor::AnchorFlags::Bottom},
                                        });
        break;
    case WM_DESTROY:
        ctx->hwnd = nullptr;
        break;
    case MUPM_RUNNING_STATE_CHANGED:
        {
            const bool running = ctx->env != nullptr;
            const auto start_hwnd = GetDlgItem(hwnd, IDC_START);
            const auto stop_hwnd = GetDlgItem(hwnd, IDC_STOP);

            Button_SetText(start_hwnd, running ? L"Restart" : L"Start");
            Button_Enable(stop_hwnd, running);

            break;
        }
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_BROWSE:
            {
                const auto path = FilePicker::show_open_dialog(L"o_lua", hwnd, L"*.lua");
                if (path.empty())
                {
                    break;
                }

                ctx->typed_path = path;

                Edit_SetText(GetDlgItem(hwnd, IDC_PATH), path.c_str());
                break;
            }
        case IDC_START:
            {
                wchar_t path[MAX_PATH]{};
                Edit_GetText(GetDlgItem(ctx->hwnd, IDC_PATH), path, std::size(path));

                start(*ctx, path);
                break;
            }
        case IDC_STOP:
            stop(*ctx);
            break;
        case IDC_CLEAR:
            ctx->logs = L"";
            Edit_SetText(GetDlgItem(hwnd, IDC_LOG), ctx->logs.c_str());
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    // lparam is a lua environment pointer
    return FALSE;
}

INT_PTR CALLBACK lua_manager_dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            // Grow the manager dialog to fit the instance dialog (we need to manually load the template and read its width)
            DLGTEMPLATEEX* dlg_template{};
            load_resource_as_dialog_template(IDD_LUA_INSTANCE, &dlg_template);

            RECT dlg_rect = {0, 0, dlg_template->cx, dlg_template->cy};
            MapDialogRect(hwnd, &dlg_rect);

            RECT mgr_rc{};
            GetClientRect(hwnd, &mgr_rc);

            RECT effective_rc = mgr_rc;
            AdjustWindowRect(&effective_rc, GetWindowLong(hwnd, GWL_STYLE), FALSE);

            SetWindowPos(hwnd, 0, 0, 0, effective_rc.right + dlg_rect.right, effective_rc.bottom, SWP_NOMOVE | SWP_NOZORDER);

            g_dlg.initial_rect = mgr_rc;

            PostMessage(hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);

            ResizeAnchor::add_anchors(hwnd, {{GetDlgItem(hwnd, IDC_ADD_INSTANCE), ResizeAnchor::AnchorFlags::Bottom}, {GetDlgItem(hwnd, IDC_INSTANCES), ResizeAnchor::AnchorFlags::Top | ResizeAnchor::AnchorFlags::Bottom}});

            return TRUE;
        }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return TRUE;
    case WM_DESTROY:
        DestroyWindow(g_dlg.inst_hwnd);
        g_dlg.inst_hwnd = nullptr;
        g_dlg.mgr_hwnd = nullptr;
        break;
    case MUPM_REBUILD_INSTANCE_LIST:
        {
            const auto hlb = GetDlgItem(hwnd, IDC_INSTANCES);
            ListBox_ResetContent(hlb);
            for (const auto& ctx : g_lua_instance_wnd_ctxs)
            {
                std::wstring display_name;
                if (ctx->env)
                {
                    display_name += L"* ";
                }
                display_name += ctx->typed_path.filename().wstring();

                const auto index = ListBox_AddString(hlb, display_name.c_str());
                ListBox_SetItemData(hlb, index, reinterpret_cast<LPARAM>(ctx.get()));
            }
            break;
        }
    case WM_CONTEXTMENU:
        {
            const HWND lb_hwnd = GetDlgItem(hwnd, IDC_INSTANCES);

            if (wparam != (WPARAM)lb_hwnd)
            {
                break;
            }

            const auto item_count = ListBox_GetCount(lb_hwnd);

            if (item_count == 0)
            {
                break;
            }

            const auto selected_index = ListBox_GetCurSel(lb_hwnd);
            if (selected_index == LB_ERR || selected_index >= item_count)
            {
                break;
            }

            const auto& selected_ctx = g_lua_instance_wnd_ctxs[selected_index];

            HMENU h_menu = CreatePopupMenu();
            const int disable_if_stopped = selected_ctx->env ? MF_ENABLED : MF_DISABLED;
            AppendMenu(h_menu, MF_STRING, 1, selected_ctx->env ? L"Restart" : L"Start");
            AppendMenu(h_menu, disable_if_stopped | MF_STRING, 2, L"Stop");
            AppendMenu(h_menu, MF_STRING, 3, L"Remove");
            AppendMenu(h_menu, MF_SEPARATOR, 4, L"");
            AppendMenu(h_menu, MF_STRING, 5, L"Stop All");

            const int offset = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), hwnd, nullptr);

            switch (offset)
            {
            case 1:
                start(*selected_ctx, selected_ctx->typed_path);
                break;
            case 2:
                stop(*selected_ctx);
                break;
            case 3:
                stop(*selected_ctx);
                g_lua_instance_wnd_ctxs.erase(g_lua_instance_wnd_ctxs.begin() + selected_index);
                PostMessage(hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);
                break;
            case 5:
                for (const auto& ctx : g_lua_instance_wnd_ctxs)
                {
                    stop(*ctx);
                }
                break;
            default:
                break;
            }

            break;
        }
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_INSTANCES:
            {
                switch (HIWORD(wparam))
                {
                case LBN_SELCHANGE:
                    {
                        if (IsWindow(g_dlg.inst_hwnd))
                        {
                            ResizeAnchor::remove_anchor(hwnd, g_dlg.inst_hwnd);
                            DestroyWindow(g_dlg.inst_hwnd);
                        }

                        const auto index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_INSTANCES));
                        if (index == LB_ERR || index >= g_lua_instance_wnd_ctxs.size())
                        {
                            break;
                        }

                        const auto param = g_lua_instance_wnd_ctxs[index].get();
                        g_dlg.inst_hwnd = CreateDialogParam(g_app_instance, MAKEINTRESOURCE(IDD_LUA_INSTANCE), hwnd, lua_instance_dialog_proc, (LPARAM)param);

                        SetWindowPos(g_dlg.inst_hwnd, nullptr, g_dlg.initial_rect.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

                        ResizeAnchor::add_anchors(hwnd, {{g_dlg.inst_hwnd, ResizeAnchor::AnchorFlags::Left | ResizeAnchor::AnchorFlags::Right | ResizeAnchor::AnchorFlags::Top | ResizeAnchor::AnchorFlags::Bottom}});

                        break;
                    }
                case LBN_DBLCLK:
                    {
                        const auto index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_INSTANCES));
                        if (index == LB_ERR || index >= g_lua_instance_wnd_ctxs.size())
                        {
                            break;
                        }

                        const auto& ctx = g_lua_instance_wnd_ctxs[index];

                        start(*ctx, ctx->typed_path);

                        break;
                    }
                default:
                    break;
                }

                if (IsWindow(g_dlg.inst_hwnd))
                {
                    ResizeAnchor::remove_anchor(hwnd, g_dlg.inst_hwnd);
                    DestroyWindow(g_dlg.inst_hwnd);
                }

                const auto index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_INSTANCES));
                if (index == LB_ERR || index >= g_lua_instance_wnd_ctxs.size())
                {
                    break;
                }

                const auto param = g_lua_instance_wnd_ctxs[index].get();
                g_dlg.inst_hwnd = CreateDialogParam(g_app_instance, MAKEINTRESOURCE(IDD_LUA_INSTANCE), hwnd, lua_instance_dialog_proc, (LPARAM)param);

                SetWindowPos(g_dlg.inst_hwnd, nullptr, g_dlg.initial_rect.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

                ResizeAnchor::add_anchors(hwnd, {{g_dlg.inst_hwnd, ResizeAnchor::AnchorFlags::Left | ResizeAnchor::AnchorFlags::Right | ResizeAnchor::AnchorFlags::Top | ResizeAnchor::AnchorFlags::Bottom}});

                break;
            }
        case IDC_ADD_INSTANCE:
            {
                const auto path = FilePicker::show_open_dialog(L"o_lua_instance", hwnd, L"*.lua");
                if (path.empty())
                {
                    break;
                }
                add_and_select_instance(path);
                break;
            }
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void LuaDialog::show()
{
    if (g_dlg.mgr_hwnd)
    {
        BringWindowToTop(g_dlg.mgr_hwnd);
        return;
    }
    g_dlg.mgr_hwnd = CreateDialog(g_app_instance, MAKEINTRESOURCE(IDD_LUA_MANAGER), g_main_hwnd, lua_manager_dialog_proc);
    ShowWindow(g_dlg.mgr_hwnd, SW_SHOW);
}

void LuaDialog::add_and_start(const std::filesystem::path& path)
{
    show();
    const auto ctx = add_and_select_instance(path);
    start(*ctx, path);
}

void LuaDialog::stop_all()
{
    for (const auto& ctx : g_lua_instance_wnd_ctxs)
    {
        stop(*ctx.get());
    }
}

void LuaDialog::close_all()
{
    stop_all();
    g_lua_instance_wnd_ctxs.clear();
    SendMessage(g_dlg.mgr_hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);
    ListBox_SetCurSel(GetDlgItem(g_dlg.mgr_hwnd, IDC_INSTANCES), 0);
    SendMessage(g_dlg.mgr_hwnd, WM_COMMAND, MAKEWPARAM(IDC_INSTANCES, LBN_SELCHANGE), 0);
}

void LuaDialog::store_running_scripts()
{
    g_dlg.stored_contexts.clear();
    for (const auto& ctx : g_lua_instance_wnd_ctxs)
    {
        if (!ctx->env)
        {
            continue;
        }
        g_dlg.stored_contexts.emplace_back(ctx);
    }
}

void LuaDialog::load_running_scripts()
{
    for (const auto& ctx : g_dlg.stored_contexts)
    {
        start(*ctx, ctx->typed_path);
    }
    g_dlg.stored_contexts.clear();
}

void LuaDialog::print(const t_lua_environment& ctx, const std::wstring& text)
{
    // Find the context for the given Lua environment
    for (const auto& wnd_ctx : g_lua_instance_wnd_ctxs)
    {
        if (!wnd_ctx->env || wnd_ctx->env != &ctx)
        {
            continue;
        }

        ::print(*wnd_ctx, text);

        return;
    }
}

HWND LuaDialog::hwnd()
{
    return g_dlg.mgr_hwnd;
}
