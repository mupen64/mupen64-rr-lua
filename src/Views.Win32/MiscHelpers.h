/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Records the execution time of a scope
 */
class ScopeTimer {
public:
    ScopeTimer(const std::string& name, spdlog::logger* logger)
    {
        m_name = name;
        m_logger = logger;
        m_start_time = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer()
    {
        print_duration();
    }

    void print_duration() const
    {
        m_logger->info("[ScopeTimer] {}: {}ms", m_name.c_str(), momentary_ms());
    }

    [[nodiscard]] int momentary_ms() const
    {
        return static_cast<int>((std::chrono::high_resolution_clock::now() - m_start_time).count() / 1'000'000);
    }

private:
    std::string m_name;
    spdlog::logger* m_logger;
    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
};

static RECT get_window_rect_client_space(HWND parent, HWND child)
{
    RECT offset_client = {0};
    MapWindowRect(child, parent, &offset_client);

    RECT client = {0};
    GetWindowRect(child, &client);

    return {
    offset_client.left,
    offset_client.top,
    offset_client.left + (client.right - client.left),
    offset_client.top + (client.bottom - client.top)};
}
static bool create_composition_surface(HWND hwnd, D2D1_SIZE_U size, IDXGIFactory2** factory, IDXGIAdapter1** dxgiadapter, ID3D11Device** d3device, IDXGIDevice1** dxdevice, ID2D1Bitmap1** bitmap, IDCompositionVisual** comp_visual, IDCompositionDevice** comp_device, IDCompositionTarget** comp_target, IDXGISwapChain1** swapchain, ID2D1Factory3** d2d_factory, ID2D1Device2** d2d_device, ID3D11DeviceContext** d3d_dc, ID2D1DeviceContext2** d2d_dc, IDXGISurface1** dxgi_surface, ID3D11Resource** dxgi_surface_resource, ID3D11Resource** front_buffer, ID3D11Texture2D** d3d_gdi_tex)
{
    CreateDXGIFactory2(0, IID_PPV_ARGS(factory));
    (*factory)->EnumAdapters1(0, dxgiadapter);

    D3D11CreateDevice(*dxgiadapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0, D3D11_SDK_VERSION, d3device, nullptr, d3d_dc);

    (*d3device)->QueryInterface(dxdevice);
    (*dxdevice)->SetMaximumFrameLatency(1);

    DCompositionCreateDevice(*dxdevice, IID_PPV_ARGS(comp_device));
    (*comp_device)->CreateTargetForHwnd(hwnd, true, comp_target);
    (*comp_device)->CreateVisual(comp_visual);

    DXGI_SWAP_CHAIN_DESC1 swapdesc{};
    swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    swapdesc.SampleDesc.Count = 1;
    swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapdesc.BufferCount = 2;
    swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapdesc.Width = size.width;
    swapdesc.Height = size.height;

    (*factory)->CreateSwapChainForComposition(*d3device, &swapdesc, nullptr, swapchain);
    (*comp_visual)->SetContent(*swapchain);
    (*comp_target)->SetRoot(*comp_visual);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, {}, d2d_factory);
    (*d2d_factory)->CreateDevice(*dxdevice, d2d_device);
    (*d2d_device)->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_dc);

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

    (*d3device)->CreateTexture2D(&desc, nullptr, d3d_gdi_tex);
    (*d3d_gdi_tex)->QueryInterface(dxgi_surface);

    const UINT dpi = GetDpiForWindow(hwnd);
    const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
    dpi,
    dpi);

    (*d2d_dc)->CreateBitmapFromDxgiSurface(*dxgi_surface, props, bitmap);
    (*d2d_dc)->SetTarget(*bitmap);

    (*swapchain)->GetBuffer(1, IID_PPV_ARGS(front_buffer));
    (*dxgi_surface)->QueryInterface(dxgi_surface_resource);

    return true;
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
static void copy_to_clipboard(void* owner, const std::wstring& str)
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
 * \remark https://github.com/dotnet/winforms/blob/c9a58e92a1d0140bb4f91691db8055bcd91524f8/src/System.Windows.Forms/src/System/Windows/Forms/Controls/ListView/ListView.SelectedListViewItemCollection.cs#L33
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
 * Shifts the listview selection by the specified amount. Retains sparse selections. Selections which fall outside the bounds of the item range after shifting are dropped.
 * \param hwnd Handle to a listview.
 * \param offset The shift amount.
 */
static void shift_listview_selection(const HWND hwnd, const int32_t offset)
{
    auto raw_selection = get_listview_selection(hwnd);
    std::vector<int64_t> selection(raw_selection.begin(), raw_selection.end());

    for (const auto selected_index : selection)
    {
        ListView_SetItemState(hwnd, selected_index, 0, LVIS_SELECTED);
    }

    for (auto& selected_index : selection)
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

    for (const auto& idx : selection)
    {
        ListView_SetItemState(hwnd, idx, 0, LVIS_SELECTED);
    }

    for (const auto& idx : indicies)
    {
        ListView_SetItemState(hwnd, idx, LVIS_SELECTED, LVIS_SELECTED);
    }
}


/**
 * \brief Initializes COM within the object's scope for the current thread
 */
class COMInitializer {
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
    const HANDLE h_find = FindFirstFile((directory + L"*").c_str(),
                                        &find_file_data);
    if (h_find == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    std::vector<std::wstring> paths;
    std::wstring fixed_path = directory;
    do
    {
        if (!lstrcmpW(find_file_data.cFileName, L".") || !lstrcmpW(find_file_data.cFileName, L".."))
            continue;

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
        for (const auto& path : get_files_in_subdirectories(full_path + L"\\"))
        {
            paths.push_back(path);
        }
    }
    while (FindNextFile(h_find, &find_file_data) != 0);

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
 * \brief Limits a value to a specific range
 * \param value The value to limit
 * \param min The lower bound
 * \param max The upper bound
 * \return The value, limited to the specified range
 */
template <typename T>
static T clamp(const T value, const T min, const T max)
{
    if (value > max)
    {
        return max;
    }
    if (value < min)
    {
        return min;
    }
    return value;
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
    const auto data = static_cast<const char*>(LockResource(rc_data));
    return std::string(data, size);
}
