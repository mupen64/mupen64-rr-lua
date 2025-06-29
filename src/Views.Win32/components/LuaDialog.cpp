#include "stdafx.h"
#include <components/FilePicker.h>
#include <components/LuaDialog.h>
#include <lua/LuaConsole.h>

#define MUPM_RUNNING_STATE_CHANGED (WM_USER + 24)
#define MUPM_REBUILD_INSTANCE_LIST (WM_USER + 25)

struct t_dialog_state {
    HWND mgr_hwnd{};
    HWND inst_hwnd{};
    RECT initial_rect{};
    std::vector<std::shared_ptr<t_lua_wnd_ctx>> stored_running_environments{};
};

static t_dialog_state g_lua_mgr{};
static std::vector<std::shared_ptr<t_lua_wnd_ctx>> g_lua_instance_wnd_ctxs{};

static void lua_instance_stop(const t_lua_wnd_ctx& ctx)
{
    if (!ctx.env)
    {
        return;
    }
    destroy_lua_environment(ctx.env);
}

static void lua_instance_start(t_lua_wnd_ctx* ctx, const std::filesystem::path& path)
{
    SendMessage(ctx->hwnd, WM_COMMAND, MAKEWPARAM(IDC_STOP, BN_CLICKED), 0);

    const auto error = create_lua_environment(path, ctx, [=](const std::wstring& text) {
        LuaDialog::print(*ctx, text);
    });

    if (!error.empty())
    {
        LuaDialog::print(*ctx, io_service.string_to_wstring(error));
        return;
    }

    PostMessage(ctx->hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
}

static std::shared_ptr<t_lua_wnd_ctx> add_and_select_instance(const std::filesystem::path& path)
{
    const auto ctx = std::make_shared<t_lua_wnd_ctx>();
    ctx->path = path;
    ctx->destroyed = [=] {
        PostMessage(ctx->hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
    };

    g_lua_instance_wnd_ctxs.insert(g_lua_instance_wnd_ctxs.begin(), ctx);

    if (!IsWindow(g_lua_mgr.mgr_hwnd))
    {
        return ctx;
    }

    SendMessage(g_lua_mgr.mgr_hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);
    ListBox_SetCurSel(GetDlgItem(g_lua_mgr.mgr_hwnd, IDC_INSTANCES), 0);
    SendMessage(g_lua_mgr.mgr_hwnd, WM_COMMAND, MAKEWPARAM(IDC_INSTANCES, LBN_SELCHANGE), 0);

    return ctx;
}

INT_PTR CALLBACK lua_instance_dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto ctx = (t_lua_wnd_ctx*)GetWindowLongPtr(hwnd, GWL_USERDATA);

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, GWL_USERDATA, lparam);

        ctx = (t_lua_wnd_ctx*)GetWindowLongPtr(hwnd, GWL_USERDATA);

        ctx->hwnd = hwnd;

        Edit_SetText(GetDlgItem(hwnd, IDC_PATH), ctx->path.c_str());
        Edit_SetText(GetDlgItem(hwnd, IDC_LOG), ctx->logs.c_str());

        PostMessage(hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
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

                ctx->path = path;

                Edit_SetText(GetDlgItem(hwnd, IDC_PATH), path.c_str());
                break;
            }
        case IDC_START:
            {
                wchar_t path[MAX_PATH]{};
                Edit_GetText(GetDlgItem(ctx->hwnd, IDC_PATH), path, std::size(path));

                lua_instance_start(ctx, path);
                break;
            }
        case IDC_STOP:
            lua_instance_stop(*ctx);
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

            g_lua_mgr.initial_rect = mgr_rc;

            PostMessage(hwnd, MUPM_REBUILD_INSTANCE_LIST, 0, 0);

            return TRUE;
        }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return TRUE;
    case WM_DESTROY:
        DestroyWindow(g_lua_mgr.inst_hwnd);
        g_lua_mgr.inst_hwnd = nullptr;
        g_lua_mgr.mgr_hwnd = nullptr;
        break;
    case MUPM_REBUILD_INSTANCE_LIST:
        {
            const auto hlb = GetDlgItem(hwnd, IDC_INSTANCES);
            ListBox_ResetContent(hlb);
            for (const auto& ctx : g_lua_instance_wnd_ctxs)
            {
                const auto index = ListBox_AddString(hlb, ctx->path.stem().c_str());
                ListBox_SetItemData(hlb, index, reinterpret_cast<LPARAM>(ctx.get()));
            }
            break;
        }
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_INSTANCES:
            {
                if (HIWORD(wparam) != LBN_SELCHANGE)
                {
                    break;
                }

                const auto index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_INSTANCES));
                if (index == LB_ERR || index >= g_lua_instance_wnd_ctxs.size())
                {
                    break;
                }

                if (IsWindow(g_lua_mgr.inst_hwnd))
                {
                    DestroyWindow(g_lua_mgr.inst_hwnd);
                }

                const auto param = g_lua_instance_wnd_ctxs[index].get();
                g_lua_mgr.inst_hwnd = CreateDialogParam(g_app_instance, MAKEINTRESOURCE(IDD_LUA_INSTANCE), hwnd, lua_instance_dialog_proc, (LPARAM)param);

                SetWindowPos(g_lua_mgr.inst_hwnd, nullptr, g_lua_mgr.initial_rect.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

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
    if (g_lua_mgr.mgr_hwnd)
    {
        BringWindowToTop(g_lua_mgr.mgr_hwnd);
        return;
    }
    g_lua_mgr.mgr_hwnd = CreateDialog(g_app_instance, MAKEINTRESOURCE(IDD_LUA_MANAGER), g_main_hwnd, lua_manager_dialog_proc);
    ShowWindow(g_lua_mgr.mgr_hwnd, SW_SHOW);
}

void LuaDialog::add_and_start(const std::filesystem::path& path)
{
    show();
    const auto ctx = add_and_select_instance(path);
    lua_instance_start(ctx.get(), path);
}

void LuaDialog::stop_all()
{
    for (const auto& ctx : g_lua_instance_wnd_ctxs)
    {
        lua_instance_stop(*ctx.get());
    }
}

void LuaDialog::close_all()
{
    stop_all();
    g_lua_instance_wnd_ctxs.clear();
}

void LuaDialog::store_running_scripts()
{
    g_lua_mgr.stored_running_environments.clear();
    for (const auto& ctx : g_lua_instance_wnd_ctxs)
    {
        if (!ctx->env)
        {
            continue;
        }
        g_lua_mgr.stored_running_environments.emplace_back(ctx);
    }
}

void LuaDialog::load_running_scripts()
{
    for (const auto& ctx : g_lua_mgr.stored_running_environments)
    {
        lua_instance_start(ctx.get(), ctx->path);
    }
    g_lua_mgr.stored_running_environments.clear();
}

void LuaDialog::print(t_lua_wnd_ctx& ctx, const std::wstring& text)
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
