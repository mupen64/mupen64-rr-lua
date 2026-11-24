/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <components/MGECompositor.hpp>

#include <Common/Assert.hpp>

#include <plugin/Plugin.hpp>
#include <Common.Views/Messages.hpp>
#include <lua/LuaRealmManager.hpp>

#include <mutex>
#include <optional>

constexpr auto control_class_name = "game_control";
constexpr DXGI_FORMAT texture_format = DXGI_FORMAT_B8G8R8A8_UNORM;

const std::string VERTEX_SHADER = R"(
    cbuffer CB : register(b0) {}
    struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
    VSOut main(uint vid : SV_VertexID)
    {
        // 6 vertices to form two triangles covering the screen
        float2 pos[6] = {
            float2(-1.0,  1.0),
            float2( 1.0,  1.0),
            float2(-1.0, -1.0),

            float2(-1.0, -1.0),
            float2( 1.0,  1.0),
            float2( 1.0, -1.0)
        };
        float2 uv[6] = {
            float2(0.0, 1.0),
            float2(1.0, 1.0),
            float2(0.0, 0.0),

            float2(0.0, 0.0),
            float2(1.0, 1.0),
            float2(1.0, 0.0)
        };
        VSOut o;
        o.pos = float4(pos[vid], 0.0, 1.0);
        o.uv = uv[vid];
        return o;
    }
    )";

const std::string FRAGMENT_SHADER = R"(
    Texture2D tex : register(t0);
    SamplerState samp : register(s0);
    struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
    float4 main(PSIn input) : SV_TARGET
    {
        float4 c = tex.Sample(samp, input.uv);
        return c;
    }
    )";

struct MgeContext
{
    int32_t last_width{};
    int32_t last_height{};
    int32_t width{};
    int32_t height{};
    void *rgba_buffer{};

    HWND hwnd{};

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    std::optional<uint64_t> shown_sequence;
};

static std::mutex g_mutex;

static MgeContext mge_context{};

static void create_d3d(const HWND hwnd)
{
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};

    ID3D11Device *device_raw{};
    ID3D11DeviceContext *context_raw{};

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, feature_levels,
        ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &device_raw, nullptr, &context_raw);
    need(hr, "D3D11CreateDevice");

    mge_context.device.Attach(device_raw);
    mge_context.context.Attach(context_raw);

    ComPtr<IDXGIDevice> dxgi_device;
    hr = mge_context.device.As(&dxgi_device);
    need(hr, "Get IDXGIDevice");

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgi_device->GetAdapter(&adapter);
    need(hr, "GetAdapter");

    ComPtr<IDXGIFactory2> factory;
    hr = adapter->GetParent(IID_PPV_ARGS(&factory));
    need(hr, "GetParent IDXGIFactory2");

    DXGI_SWAP_CHAIN_DESC1 scdesc = {};
    scdesc.Width = (UINT)mge_context.width;
    scdesc.Height = (UINT)mge_context.height;
    scdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scdesc.SampleDesc.Count = 1;
    scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scdesc.BufferCount = 2;
    scdesc.Scaling = DXGI_SCALING_STRETCH;
    scdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scdesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    scdesc.Flags = 0;

    IDXGISwapChain1 *swap_raw{};
    hr = factory->CreateSwapChainForHwnd(mge_context.device.Get(), hwnd, &scdesc, nullptr, nullptr, &swap_raw);
    need(hr, "CreateSwapChainForHwnd");

    mge_context.swapchain.Attach(swap_raw);

    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    // Point sampler for nearest-neighbour scaling
    D3D11_SAMPLER_DESC sampdesc = {};
    sampdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampdesc.MinLOD = 0;
    sampdesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = mge_context.device->CreateSamplerState(&sampdesc, &mge_context.sampler);
    need(hr, "CreateSamplerState");

    ComPtr<ID3DBlob> vs_blob, ps_blob, err_blob;
    hr = D3DCompile(VERTEX_SHADER.data(), VERTEX_SHADER.size(), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0,
        &vs_blob, &err_blob);
    need(hr, "D3DCompile");

    hr = D3DCompile(FRAGMENT_SHADER.data(), FRAGMENT_SHADER.size(), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0,
        &ps_blob, &err_blob);
    need(hr, "D3DCompile");

    hr = mge_context.device->CreateVertexShader(
        vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &mge_context.vs);
    need(hr, "CreateVertexShader");

    hr = mge_context.device->CreatePixelShader(
        ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &mge_context.ps);
    need(hr, "CreatePixelShader");

    // Set up the pipeline
    mge_context.context->VSSetShader(mge_context.vs.Get(), nullptr, 0);
    mge_context.context->PSSetShader(mge_context.ps.Get(), nullptr, 0);
    ID3D11SamplerState *samps[] = {mge_context.sampler.Get()};
    mge_context.context->PSSetSamplers(0, 1, samps);
    mge_context.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void destroy_d3d()
{
    mge_context.srv.Reset();
    mge_context.texture.Reset();
    mge_context.rtv.Reset();
    mge_context.swapchain.Reset();
    mge_context.context.Reset();
    mge_context.device.Reset();
    mge_context.vs.Reset();
    mge_context.ps.Reset();
    mge_context.sampler.Reset();
}

static void ensure_texture_exists_with_size(const int w, const int h)
{
    if (mge_context.texture && mge_context.last_width == w && mge_context.last_height == h) return;

    mge_context.srv.Reset();
    mge_context.texture.Reset();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = (UINT)w;
    desc.Height = (UINT)h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = texture_format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    HRESULT hr = mge_context.device->CreateTexture2D(&desc, nullptr, &mge_context.texture);
    need(hr, "CreateTexture2D");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = desc.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    hr = mge_context.device->CreateShaderResourceView(mge_context.texture.Get(), &srvd, &mge_context.srv);
    need(hr, "CreateShaderResourceView");

    mge_context.last_width = w;
    mge_context.last_height = h;
}

static void upload_rgb32_buffer()
{
    if (!mge_context.texture) return;

    D3D11_BOX db = {};
    db.left = 0;
    db.top = 0;
    db.front = 0;
    db.right = mge_context.width;
    db.bottom = mge_context.height;
    db.back = 1;

    mge_context.context->UpdateSubresource(
        mge_context.texture.Get(), 0, &db, mge_context.rgba_buffer, mge_context.width * 4, 0);
}

static void render_and_present()
{
    need(mge_context.context && mge_context.rtv && mge_context.srv, "D3D context not initialized");

    ID3D11RenderTargetView *rtv = mge_context.rtv.Get();
    mge_context.context->OMSetRenderTargets(1, &rtv, nullptr);

    // Bind SRV
    ID3D11ShaderResourceView *srvs[] = {mge_context.srv.Get()};
    mge_context.context->PSSetShaderResources(0, 1, srvs);

    // Draw fullscreen quad
    mge_context.context->Draw(6, 0);

    // Unbind SRV to allow UpdateSubresource
    ID3D11ShaderResourceView *null_srv[1] = {nullptr};
    mge_context.context->PSSetShaderResources(0, 1, null_srv);

    mge_context.swapchain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
}

static void recreate_mge_context_d3d()
{
    g_view_logger->info("Creating MGE (D3D11) context with size {}x{}...", mge_context.width, mge_context.height);

    if (!mge_context.device)
    {
        create_d3d(mge_context.hwnd);
    }
    else
    {
        mge_context.context->OMSetRenderTargets(0, nullptr, nullptr);
        mge_context.rtv.Reset();
        const HRESULT hr = mge_context.swapchain->ResizeBuffers(
            0, (UINT)mge_context.width, (UINT)mge_context.height, DXGI_FORMAT_UNKNOWN, 0);
        need(hr, "ResizeBuffers");
    }

    ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    need(hr, "GetBuffer");

    hr = mge_context.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &mge_context.rtv);
    need(hr, "CreateRenderTargetView");

    D3D11_TEXTURE2D_DESC desc;
    back_buffer->GetDesc(&desc);
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(desc.Width);
    viewport.Height = static_cast<float>(desc.Height);
    viewport.MaxDepth = 1.0f;
    mge_context.context->RSSetViewports(1, &viewport);

    _aligned_free(mge_context.rgba_buffer);

    const auto buffer_size = mge_context.width * mge_context.height * 4;

    mge_context.rgba_buffer = _aligned_malloc(buffer_size, 16);
    need(mge_context.rgba_buffer, "Failed to allocate MGE buffers");

    ZeroMemory(mge_context.rgba_buffer, buffer_size);
    mge_context.shown_sequence.reset();

    ensure_texture_exists_with_size(mge_context.width, mge_context.height);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_SIZE:
        return 0;
    case WM_NCDESTROY: {
        std::lock_guard lock(g_mutex);
        destroy_d3d();
        break;
    }
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void MGECompositor::create(HWND hwnd)
{
    mge_context.hwnd = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, control_class_name, "",
        WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 1, 1, hwnd, nullptr, g_main_ctx.hinst, nullptr);
}

void MGECompositor::init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wndclass.lpfnWndProc = (WNDPROC)wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = control_class_name;
    RegisterClass(&wndclass);

    Messenger::subscribe<Messenger::Message::EmuLaunchedChanged>([](bool value) {
        if (value && PluginUtil::mge_available())
            SetWindowPos(
                mge_context.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        else
            ShowWindow(mge_context.hwnd, SW_HIDE);
    });
}

void MGECompositor::update_screen()
{
    std::lock_guard lock(g_mutex);

    PluginUtil::get_video_size(&mge_context.width, &mge_context.height);

    if (mge_context.width != mge_context.last_width || mge_context.height != mge_context.last_height)
    {
        SetWindowPos(mge_context.hwnd, nullptr, 0, 0, mge_context.width, mge_context.height,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
        recreate_mge_context_d3d();
    }

    // Read before the frame, so that a frame produced meanwhile is read again next time rather than skipped.
    const auto sequence = PluginUtil::get_video_sequence();
    if (sequence && sequence == mge_context.shown_sequence) return;

    PluginUtil::read_video(mge_context.rgba_buffer);

    upload_rgb32_buffer();
    render_and_present();

    mge_context.shown_sequence = sequence;
    mge_context.last_width = mge_context.width;
    mge_context.last_height = mge_context.height;
}

HWND MGECompositor::hwnd()
{
    return mge_context.hwnd;
}

void MGECompositor::get_video_size(int32_t *width, int32_t *height)
{
    std::lock_guard lock(g_mutex);

    if (width)
    {
        *width = mge_context.width;
    }
    if (height)
    {
        *height = mge_context.height;
    }
}

void MGECompositor::copy_video(void *buffer)
{
    std::lock_guard lock(g_mutex);

    memcpy(buffer, mge_context.rgba_buffer, mge_context.width * mge_context.height * 4);
}

void MGECompositor::load_screen(void *data)
{
    std::lock_guard lock(g_mutex);

    memcpy(mge_context.rgba_buffer, data, mge_context.width * mge_context.height * 4);

    ensure_texture_exists_with_size(mge_context.width, mge_context.height);
    upload_rgb32_buffer();
    render_and_present();

    mge_context.shown_sequence = PluginUtil::get_video_sequence();
}
