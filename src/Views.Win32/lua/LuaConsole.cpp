/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "LuaConsole.h"
#include "Config.h"
#include "DialogService.h"
#include "LuaCallbacks.h"
#include "LuaRegistry.h"
#include "Messenger.h"
#include <components/FilePicker.h>
#include <lua/LuaRenderer.h>

#define MUPM_RUNNING_STATE_CHANGED (WM_USER + 24)
#define MUPM_REBUILD_INSTANCE_LIST (WM_USER + 25)

struct t_lua_manager_state {
    HWND mgr_hwnd{};
    HWND inst_hwnd{};
    RECT initial_rect{};
    std::vector<std::shared_ptr<t_lua_wnd_ctx>> stored_running_environments{};
};

const auto INSTANCE_CTX_PROP = L"mup_lua_prop";

core_buttons last_controller_data[4];
core_buttons new_controller_data[4];
bool overwrite_controller_data[4];
size_t g_input_count = 0;

std::vector<t_lua_environment*> g_lua_environments;
std::unordered_map<lua_State*, t_lua_environment*> g_lua_env_map;

std::string mupen_api_lua_code;
std::string inspect_lua_code;
std::string shims_lua_code;

t_lua_manager_state g_lua_mgr = {};
std::vector<std::shared_ptr<t_lua_wnd_ctx>> g_lua_instance_wnd_ctxs{};

t_lua_environment* get_lua_class(lua_State* lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}

int at_panic(lua_State* L)
{
    const auto message = io_service.string_to_wstring(lua_tostring(L, -1));

    g_view_logger->info(L"Lua panic: {}", message);
    DialogService::show_dialog(message.c_str(), L"Lua", fsvc_error);

    return 0;
}

void print_con(t_lua_wnd_ctx& ctx, const std::wstring& text)
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

void print_con(const t_lua_environment& env, const std::wstring& text)
{
    print_con(*env.wnd_ctx, text);
}

static std::shared_ptr<t_lua_wnd_ctx> add_and_select_instance(const std::filesystem::path& path)
{
    const auto ctx = std::make_shared<t_lua_wnd_ctx>();
    ctx->path = path;

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

    const auto error = create_lua_environment(path, ctx);

    if (!error.empty())
    {
        print_con(*ctx, io_service.string_to_wstring(error));
        return;
    }

    PostMessage(ctx->hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
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

void lua_close_all_scripts()
{
    assert(is_on_gui_thread());

    lua_stop_all_scripts();

    g_lua_instance_wnd_ctxs = {};
}

void lua_stop_all_scripts()
{
    assert(is_on_gui_thread());

    for (const auto& ctx : g_lua_instance_wnd_ctxs)
    {
        SendMessage(ctx->hwnd, WM_COMMAND, MAKEWPARAM(IDC_STOP, BN_CLICKED), 0);
    }
}

void lua_init()
{
    mupen_api_lua_code = load_resource_as_string(IDR_API_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    inspect_lua_code = load_resource_as_string(IDR_INSPECT_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    shims_lua_code = load_resource_as_string(IDR_SHIMS_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
}

void LuaManager::show_manager_dialog()
{
    if (g_lua_mgr.mgr_hwnd)
    {
        BringWindowToTop(g_lua_mgr.mgr_hwnd);
        return;
    }
    g_lua_mgr.mgr_hwnd = CreateDialog(g_app_instance, MAKEINTRESOURCE(IDD_LUA_MANAGER), g_main_hwnd, lua_manager_dialog_proc);
    ShowWindow(g_lua_mgr.mgr_hwnd, SW_SHOW);
}

void LuaManager::add_and_run(const std::filesystem::path& path)
{
    assert(is_on_gui_thread());

    show_manager_dialog();
    const auto ctx = add_and_select_instance(path);
    lua_instance_start(ctx.get(), path);
}

void LuaManager::save_running_scripts()
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

void LuaManager::recall_and_start_scripts()
{
    for (const auto& ctx : g_lua_mgr.stored_running_environments)
    {
        lua_instance_start(ctx.get(), ctx->path);
    }
    g_lua_mgr.stored_running_environments.clear();
}

static void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto& lua : g_lua_environments)
    {
        g_lua_env_map[lua->L] = lua;
    }
}

void destroy_lua_environment(t_lua_environment* lua)
{
    LuaRenderer::pre_destroy_renderer(&lua->rctx);

    LuaCallbacks::invoke_callbacks_with_key(*lua, LuaCallbacks::REG_ATSTOP);

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const t_lua_environment* v) {
        return v == lua;
    });
    lua->wnd_ctx->env = nullptr;
    rebuild_lua_env_map();

    lua_close(lua->L);
    lua->L = nullptr;
    PostMessage(lua->wnd_ctx->hwnd, MUPM_RUNNING_STATE_CHANGED, 0, 0);
    LuaRenderer::destroy_renderer(&lua->rctx);

    g_view_logger->info("Lua destroyed");
}


std::string create_lua_environment(const std::filesystem::path& path, t_lua_wnd_ctx* inst_wnd_ctx)
{
    assert(is_on_gui_thread());

    auto lua = new t_lua_environment();

    lua->path = path;
    lua->wnd_ctx = inst_wnd_ctx;
    lua->rctx = LuaRenderer::default_rendering_context();

    lua->L = luaL_newstate();
    lua_atpanic(lua->L, at_panic);
    LuaRegistry::register_functions(lua->L);
    LuaRenderer::create_renderer(&lua->rctx, lua);

    // NOTE: We need to add the lua to the global map already since it may receive callbacks while its executing the global code
    g_lua_environments.push_back(lua);
    inst_wnd_ctx->env = lua;
    rebuild_lua_env_map();

    bool has_error = false;

    {
        ScopeTimer timer("mupenapi.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, mupen_api_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    LuaRegistry::register_functions(lua->L);

    {
        ScopeTimer timer("inspect.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, inspect_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    {
        ScopeTimer timer("shims.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, shims_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    if (luaL_dofile(lua->L, lua->path.string().c_str()))
    {
        has_error = true;
    }

    std::string error_msg;
    if (has_error)
    {
        g_lua_environments.pop_back();
        inst_wnd_ctx->env = nullptr;
        rebuild_lua_env_map();

        error_msg = lua_tostring(lua->L, -1);
        destroy_lua_environment(lua);
        delete lua;
        lua = nullptr;
    }

    return error_msg;
}


void* lua_tocallback(lua_State* L, const int i)
{
    void* key = calloc(1, sizeof(void*));
    lua_pushvalue(L, i);
    lua_pushlightuserdata(L, key);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    return key;
}

void lua_pushcallback(lua_State* L, void* key)
{
    lua_pushlightuserdata(L, key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    free(key);
    key = nullptr;
}
