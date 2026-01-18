/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <DialogService.h>

typedef struct
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
    uint16_t *menu;
    uint16_t *windowClass;
    WCHAR *title;
    WORD pointsize;
    WORD weight;
    BYTE italic;
    BYTE charset;
    WCHAR *typeface;
} DLGTEMPLATEEX;

/**
 * \brief Records the execution time of a scope
 */
class ScopeTimer
{
  public:
    ScopeTimer(const std::string &name, spdlog::logger *logger)
    {
        m_name = name;
        m_logger = logger;
        m_start_time = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer() { print_duration(); }

    void print_duration() const { m_logger->info("[ScopeTimer] {}: {}ms", m_name.c_str(), momentary_ms()); }

    [[nodiscard]] int momentary_ms() const
    {
        return static_cast<int>((std::chrono::high_resolution_clock::now() - m_start_time).count() / 1'000'000);
    }

  private:
    std::string m_name;
    spdlog::logger *m_logger;
    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
};

class WindowDisabler
{
  public:
    explicit WindowDisabler(const HWND hwnd) : m_hwnd(hwnd)
    {
        if (!IsWindow(hwnd))
        {
            m_hwnd = nullptr;
            return;
        }
        m_prev_enabled = IsWindowEnabled(m_hwnd);
        EnableWindow(hwnd, FALSE);
    }

    ~WindowDisabler()
    {
        if (IsWindow(m_hwnd))
        {
            EnableWindow(m_hwnd, m_prev_enabled);
        }
    }

  private:
    HWND m_hwnd{};
    bool m_prev_enabled{};
};

#define ComboBox_ResetContentKeepEdit(hwnd)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        while (ComboBox_GetCount(hwnd) > 0) ComboBox_DeleteString(hwnd, 0);                                            \
    } while (0)

static void runtime_assert_fail(const std::wstring &message)
{
#if defined(_DEBUG)
    __debugbreak();
#endif
    DialogService::show_dialog(message.c_str(), L"Failed Runtime Assertion", fsvc_error);
    std::terminate();
}

/**
 * \brief Asserts a condition at runtime.
 */
#define RT_ASSERT(condition, message)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            runtime_assert_fail(message);                                                                              \
        }                                                                                                              \
    } while (0)

/**
 * \brief Asserts that an HRESULT is SUCCESS at runtime.
 */
#define RT_ASSERT_HR(hr, message) RT_ASSERT(!FAILED(hr), message)

static RECT get_window_rect_client_space(HWND parent, HWND child)
{
    RECT offset_client = {0};
    MapWindowRect(child, parent, &offset_client);

    RECT client = {0};
    GetWindowRect(child, &client);

    return {offset_client.left, offset_client.top, offset_client.left + (client.right - client.left),
            offset_client.top + (client.bottom - client.top)};
}

struct CompositionContext
{
    IDXGIFactory2 *dxgi_factory{};
    IDXGIAdapter1 *dxgi_adapter{};
    IDXGIDevice1 *dxgi_device{};
    IDXGISwapChain1 *dxgi_swapchain{};
    IDXGISurface1 *dxgi_surface{};

    ID3D11Device *d3d11_device{};
    ID3D11DeviceContext *d3d_dc{};
    ID3D11Resource *d3d11_surface{};
    ID3D11Resource *d3d11_front_buffer{};
    ID3D11Texture2D *d3d11_gdi_tex{};

    ID2D1Bitmap1 *d2d1_bitmap{};
    ID2D1Factory3 *d2d_factory{};
    ID2D1Device2 *d2d_device{};
    ID2D1DeviceContext2 *d2d_dc{};

    IDCompositionVisual *comp_visual{};
    IDCompositionDevice *comp_device{};
    IDCompositionTarget *comp_target{};
};

static std::optional<CompositionContext> create_composition_context(const HWND hwnd, const D2D1_SIZE_U size)
{
    CompositionContext ctx;

    CreateDXGIFactory2(0, IID_PPV_ARGS(&ctx.dxgi_factory));
    ctx.dxgi_factory->EnumAdapters1(0, &ctx.dxgi_adapter);

    D3D11CreateDevice(ctx.dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0,
                      D3D11_SDK_VERSION, &ctx.d3d11_device, nullptr, &ctx.d3d_dc);

    ctx.d3d11_device->QueryInterface(&ctx.dxgi_device);
    ctx.dxgi_device->SetMaximumFrameLatency(1);

    DCompositionCreateDevice(ctx.dxgi_device, IID_PPV_ARGS(&ctx.comp_device));
    ctx.comp_device->CreateTargetForHwnd(hwnd, true, &ctx.comp_target);
    ctx.comp_device->CreateVisual(&ctx.comp_visual);

    DXGI_SWAP_CHAIN_DESC1 swapdesc{};
    swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    swapdesc.SampleDesc.Count = 1;
    swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapdesc.BufferCount = 2;
    swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapdesc.Width = size.width;
    swapdesc.Height = size.height;

    ctx.dxgi_factory->CreateSwapChainForComposition(ctx.d3d11_device, &swapdesc, nullptr, &ctx.dxgi_swapchain);
    ctx.comp_visual->SetContent(ctx.dxgi_swapchain);
    ctx.comp_target->SetRoot(ctx.comp_visual);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, {}, &ctx.d2d_factory);
    ctx.d2d_factory->CreateDevice(ctx.dxgi_device, &ctx.d2d_device);
    ctx.d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &ctx.d2d_dc);

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = size.width;
    desc.Height = size.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = {.Count = 1, .Quality = 0};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    ctx.d3d11_device->CreateTexture2D(&desc, nullptr, &ctx.d3d11_gdi_tex);
    ctx.d3d11_gdi_tex->QueryInterface(&ctx.dxgi_surface);

    const UINT dpi = GetDpiForWindow(hwnd);
    const D2D1_BITMAP_PROPERTIES1 props =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

    ctx.d2d_dc->CreateBitmapFromDxgiSurface(ctx.dxgi_surface, props, &ctx.d2d1_bitmap);
    ctx.d2d_dc->SetTarget(ctx.d2d1_bitmap);

    ctx.dxgi_swapchain->GetBuffer(1, IID_PPV_ARGS(&ctx.d3d11_front_buffer));
    ctx.dxgi_surface->QueryInterface(&ctx.d3d11_surface);

    return ctx;
}

static void set_statusbar_parts(HWND hwnd, std::vector<int32_t> parts)
{
    auto new_parts = parts;
    auto accumulator = 0;
    for (int i = 0; i < parts.size(); ++i)
    {
        accumulator += parts[i];
        new_parts[i] = accumulator;
    }
    SendMessage(hwnd, SB_SETPARTS, new_parts.size(), (LPARAM)new_parts.data());
}

/**
 * \brief Copies a string to the clipboard
 * \param owner The clipboard content's owner window
 * \param str The string to be copied
 */
static void copy_to_clipboard(void *owner, const std::wstring &str)
{
    OpenClipboard((HWND)owner);
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(wchar_t));
    if (hg)
    {
        memcpy(GlobalLock(hg), str.c_str(), (str.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(hg);
        SetClipboardData(CF_UNICODETEXT, hg);
        CloseClipboard();
        GlobalFree(hg);
    }
    else
    {
        g_view_logger->info("Failed to copy");
        CloseClipboard();
    }
}

/**
 * Gets the selected indicies of a listview.
 * \param hwnd Handle to a listview.
 * \return A vector containing the selected indicies.
 * \remark
 * https://github.com/dotnet/winforms/blob/c9a58e92a1d0140bb4f91691db8055bcd91524f8/src/System.Windows.Forms/src/System/Windows/Forms/Controls/ListView/ListView.SelectedListViewItemCollection.cs#L33
 */
static std::vector<size_t> get_listview_selection(const HWND hwnd)
{
    const size_t count = ListView_GetSelectedCount(hwnd);

    std::vector<size_t> indicies(count);

    int display_index = -1;

    for (size_t i = 0; i < count; ++i)
    {
        const int fidx = ListView_GetNextItem(hwnd, display_index, LVNI_SELECTED);

        if (fidx > 0)
        {
            indicies[i] = fidx;
            display_index = fidx;
        }
    }

    return indicies;
}

/**
 * Shifts the listview selection by the specified amount. Retains sparse selections. Selections which fall outside the
 * bounds of the item range after shifting are dropped. \param hwnd Handle to a listview. \param offset The shift
 * amount.
 */
static void shift_listview_selection(const HWND hwnd, const int32_t offset)
{
    auto raw_selection = get_listview_selection(hwnd);
    std::vector<int64_t> selection(raw_selection.begin(), raw_selection.end());

    for (const auto selected_index : selection)
    {
        ListView_SetItemState(hwnd, selected_index, 0, LVIS_SELECTED);
    }

    for (auto &selected_index : selection)
    {
        selected_index = std::max(selected_index + offset, static_cast<int64_t>(0));
    }

    for (const auto selected_index : selection)
    {
        ListView_SetItemState(hwnd, selected_index, LVIS_SELECTED, LVIS_SELECTED);
    }
}

/**
 * Sets the listview selection based on a vector of indicies.
 * \param hwnd Handle to a listview.
 * \param indicies A vector containing the selected indicies.
 */
static void set_listview_selection(const HWND hwnd, const std::vector<size_t> indicies)
{
    if (!IsWindow(hwnd))
    {
        return;
    }

    auto selection = get_listview_selection(hwnd);

    for (const auto &idx : selection)
    {
        ListView_SetItemState(hwnd, idx, 0, LVIS_SELECTED);
    }

    for (const auto &idx : indicies)
    {
        ListView_SetItemState(hwnd, idx, LVIS_SELECTED, LVIS_SELECTED);
    }
}

/**
 * \brief Initializes COM within the object's scope for the current thread
 */
class COMInitializer
{
  public:
    COMInitializer()
    {
        auto hr = CoInitialize(nullptr);
        m_init = !(hr != S_OK && hr != S_FALSE && hr != RPC_E_CHANGED_MODE);

        if (!m_init)
        {
            g_view_logger->info("[COMInitializer] Failed to initialize COM");
        }
    }

    ~COMInitializer()
    {
        if (m_init)
        {
            CoUninitialize();
        }
    }

  private:
    bool m_init;
};

/**
 * \brief Gets all files under all subdirectory of a specific directory, including the directory's shallow files
 * \param directory The path joiner-terminated directory
 */
static std::vector<std::wstring> get_files_in_subdirectories(std::wstring directory)
{
    if (directory.back() != L'\\')
    {
        directory += L"\\";
    }
    WIN32_FIND_DATA find_file_data;
    const HANDLE h_find = FindFirstFile((directory + L"*").c_str(), &find_file_data);
    if (h_find == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    std::vector<std::wstring> paths;
    std::wstring fixed_path = directory;
    do
    {
        if (!lstrcmpW(find_file_data.cFileName, L".") || !lstrcmpW(find_file_data.cFileName, L"..")) continue;

        auto full_path = directory + find_file_data.cFileName;

        if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            paths.push_back(full_path);
            continue;
        }

        if (directory[directory.size() - 2] == '\0')
        {
            if (directory.back() == '\\')
            {
                fixed_path.pop_back();
                fixed_path.pop_back();
            }
        }

        if (directory.back() != '\\')
        {
            fixed_path.push_back('\\');
        }

        full_path = fixed_path + find_file_data.cFileName;
        for (const auto &path : get_files_in_subdirectories(full_path + L"\\"))
        {
            paths.push_back(path);
        }
    } while (FindNextFile(h_find, &find_file_data) != 0);

    FindClose(h_find);

    return paths;
}

/**
 * \brief Gets the path to the current user's desktop
 */
static std::wstring get_desktop_path()
{
    wchar_t path[MAX_PATH + 1] = {0};
    SHGetSpecialFolderPathW(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);
    return path;
}

/**
 * \brief Formats a duration into a string of format HH:MM:SS
 * \param seconds The duration in seconds
 * \return The formatted duration
 */
static std::wstring format_duration(size_t seconds)
{
    wchar_t str[480] = {};
    wsprintfW(str, L"%02u:%02u:%02u", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
    return str;
}

/**
 * \brief Limits a wstring to a specific length, appending "..." if it exceeds the limit.
 * \param input The input wstring.
 * \param n The maximum length.
 * \return The limited wstring.
 */
[[nodiscard]] static std::wstring limit_wstring(const std::wstring &input, const size_t n)
{
    if (input.size() <= n)
    {
        return input;
    }
    if (n <= 3)
    {
        return std::wstring(n, L'.');
    }
    return input.substr(0, n - 3) + L"...";
}

/**
 * \brief Loads a resource as a string.
 * \param id The resource id.
 * \param type The resource type.
 * \return The resource as a string, or an empty string if the resource could not be loaded.
 */
static std::string load_resource_as_string(const int id, const LPCWSTR type)
{
    const HINSTANCE hinst = GetModuleHandle(nullptr);
    const HRSRC rc = FindResource(hinst, MAKEINTRESOURCE(id), type);
    if (!rc)
    {
        return "";
    }
    const HGLOBAL rc_data = LoadResource(hinst, rc);
    const auto size = SizeofResource(hinst, rc);
    const auto data = static_cast<const char *>(LockResource(rc_data));
    return std::string(data, size);
}

/**
 * \brief Loads a resource as an extended dialog template.
 * \param id The resource id.
 * \param dlg_template A pointer to a pointer that will receive the dialog template.
 * \return Whether the resource was successfully loaded.
 */
static bool load_resource_as_dialog_template(const int id, DLGTEMPLATEEX **dlg_template)
{
    *dlg_template = nullptr;

    const HINSTANCE hinst = GetModuleHandle(nullptr);

    const HRSRC rc = FindResource(hinst, MAKEINTRESOURCE(id), RT_DIALOG);
    if (!rc)
    {
        return false;
    }

    const HGLOBAL rc_data = LoadResource(hinst, rc);
    if (!rc_data)
    {
        return false;
    }

    const auto data = static_cast<DLGTEMPLATEEX *>(LockResource(rc_data));
    if (!data)
    {
        return false;
    }

    *dlg_template = data;

    return true;
}

/**
 * \brief Formats a value according to short formatting rules.
 * \param value The value to format.
 * \return A formatted string representing the value in a short format (e.g., 1.23k for 1230).
 */
static std::wstring format_short(const uint64_t value)
{
    if (value < 1'000) return std::to_wstring(value);

    auto str = std::format(L"{:.2f}k", (double)value / 1000.0);

    while (!str.empty() && str.find('.') < str.find('k') && (str.back() == '0' || str.back() == '.')) str.pop_back();

    return str;
}

/**
 * \brief Gets the text of a window.
 * \param hwnd The handle to the window.
 * \return The text of the window, or an empty optional if the operation failed.
 */
static std::optional<std::wstring> get_window_text(const HWND hwnd)
{
    if (!IsWindow(hwnd))
    {
        return std::nullopt;
    }

    SetLastError(ERROR_SUCCESS);

    auto len = GetWindowTextLength(hwnd) + 1;

    if (len == 0)
    {
        if (GetLastError() != ERROR_SUCCESS)
        {
            return std::nullopt;
        }
        return L"";
    }

    std::wstring str(len + 1, L'\0');
    const int actual_length = GetWindowText(hwnd, str.data(), len + 1);
    str.resize(actual_length);

    return str;
}

/**
 * \brief Ensures that the specified index in the listbox is visible.
 * \param hwnd The handle to the listbox.
 * \param index The index to ensure is visible.
 */
static void listbox_ensure_visible(const HWND hwnd, const int32_t index)
{
    const int sel = ListBox_GetCurSel(hwnd);

    if (sel == LB_ERR) return;

    const int top = ListBox_GetTopIndex(hwnd);

    RECT rc;
    GetClientRect(hwnd, &rc);

    const int item_height = ListBox_GetItemHeight(hwnd, 0);
    const int visible_count = (rc.bottom - rc.top) / item_height;

    if (sel < top || sel >= top + visible_count)
    {
        ListBox_SetTopIndex(hwnd, sel);
    }
}

static LRESULT CALLBACK no_resize_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                                DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, no_resize_subclass_proc, id);
        break;
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, msg, wparam, lparam);

        switch (hit)
        {
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTTOPLEFT:
        case HTTOPRIGHT:
        case HTBOTTOM:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
            return HTCLIENT;
        }

        return hit;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static void attach_no_resize_subproc(const HWND hwnd)
{
    SetWindowSubclass(hwnd, no_resize_subclass_proc, 0, 0);
}