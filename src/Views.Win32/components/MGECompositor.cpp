/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.Views/App.hpp"
#include "Common.hpp"
#include "Main.hpp"
#include <components/MGECompositor.hpp>
#include <components/Dispatcher.hpp>

#include <Common/Assert.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <plugin/Plugin.hpp>
#include <Common.Views/Messages.hpp>
#include <string>

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

struct MgeFrame
{
    int32_t width{};
    int32_t height{};
    void *buffer{};
};

struct MgeGpu
{
    int32_t width{};
    int32_t height{};

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
};

struct MgeContext
{
    HWND hwnd{};
    std::mutex frame_mutex;
    MgeFrame frame;
    std::mutex gpu_mutex;
    MgeGpu gpu;
    std::atomic<uint64_t> requested_size;
    uint64_t last_requested_size{};
};

static MgeContext mge_context{};

static uint64_t pack_size(const int32_t width, const int32_t height)
{
    return (static_cast<uint64_t>(static_cast<uint32_t>(width)) << 32) | static_cast<uint32_t>(height);
}

static void unpack_size(const uint64_t size, int32_t &width, int32_t &height)
{
    width = static_cast<int32_t>(size >> 32);
    height = static_cast<int32_t>(size & 0xFFFFFFFF);
}

static void create_d3d(const HWND hwnd)
{
    auto &gpu = mge_context.gpu;

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

    gpu.device.Attach(device_raw);
    gpu.context.Attach(context_raw);

    ComPtr<IDXGIDevice> dxgi_device;
    hr = gpu.device.As(&dxgi_device);
    need(hr, "Get IDXGIDevice");

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgi_device->GetAdapter(&adapter);
    need(hr, "GetAdapter");

    ComPtr<IDXGIFactory2> factory;
    hr = adapter->GetParent(IID_PPV_ARGS(&factory));
    need(hr, "GetParent IDXGIFactory2");

    DXGI_SWAP_CHAIN_DESC1 scdesc = {};
    scdesc.Width = 0;
    scdesc.Height = 0;
    scdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scdesc.SampleDesc.Count = 1;
    scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scdesc.BufferCount = 2;
    scdesc.Scaling = DXGI_SCALING_STRETCH;
    scdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scdesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    scdesc.Flags = 0;

    IDXGISwapChain1 *swap_raw{};
    hr = factory->CreateSwapChainForHwnd(gpu.device.Get(), hwnd, &scdesc, nullptr, nullptr, &swap_raw);
    need(hr, "CreateSwapChainForHwnd");

    gpu.swapchain.Attach(swap_raw);

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
    hr = gpu.device->CreateSamplerState(&sampdesc, &gpu.sampler);
    need(hr, "CreateSamplerState");

    ComPtr<ID3DBlob> vs_blob, ps_blob, err_blob;
    hr = D3DCompile(VERTEX_SHADER.data(), VERTEX_SHADER.size(), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0,
        &vs_blob, &err_blob);
    need(hr, "D3DCompile");

    hr = D3DCompile(FRAGMENT_SHADER.data(), FRAGMENT_SHADER.size(), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0,
        &ps_blob, &err_blob);
    need(hr, "D3DCompile");

    hr = gpu.device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &gpu.vs);
    need(hr, "CreateVertexShader");

    hr = gpu.device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &gpu.ps);
    need(hr, "CreatePixelShader");

    // Set up the pipeline
    gpu.context->VSSetShader(gpu.vs.Get(), nullptr, 0);
    gpu.context->PSSetShader(gpu.ps.Get(), nullptr, 0);
    ID3D11SamplerState *samps[] = {gpu.sampler.Get()};
    gpu.context->PSSetSamplers(0, 1, samps);
    gpu.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void destroy_d3d()
{
    std::lock_guard gpu_lock(mge_context.gpu_mutex);
    mge_context.gpu = {};
}

static void create_texture(const int32_t w, const int32_t h)
{
    auto &gpu = mge_context.gpu;

    gpu.srv.Reset();
    gpu.texture.Reset();

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

    HRESULT hr = gpu.device->CreateTexture2D(&desc, nullptr, &gpu.texture);
    need(hr, "CreateTexture2D");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = desc.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    hr = gpu.device->CreateShaderResourceView(gpu.texture.Get(), &srvd, &gpu.srv);
    need(hr, "CreateShaderResourceView");
}

static void upload_and_present()
{
    auto &gpu = mge_context.gpu;
    const auto &frame = mge_context.frame;

    if (!gpu.swapchain || !gpu.rtv || !gpu.srv || !frame.buffer) return;
    if (gpu.width != frame.width || gpu.height != frame.height) return;

    D3D11_BOX db = {};
    db.left = 0;
    db.top = 0;
    db.front = 0;
    db.right = frame.width;
    db.bottom = frame.height;
    db.back = 1;

    gpu.context->UpdateSubresource(gpu.texture.Get(), 0, &db, frame.buffer, frame.width * 4, 0);

    ID3D11RenderTargetView *rtv = gpu.rtv.Get();
    gpu.context->OMSetRenderTargets(1, &rtv, nullptr);

    // Bind SRV
    ID3D11ShaderResourceView *srvs[] = {gpu.srv.Get()};
    gpu.context->PSSetShaderResources(0, 1, srvs);

    // Draw fullscreen quad
    gpu.context->Draw(6, 0);

    // Unbind SRV to allow UpdateSubresource
    ID3D11ShaderResourceView *null_srv[1] = {nullptr};
    gpu.context->PSSetShaderResources(0, 1, null_srv);

    gpu.swapchain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
}

static void resize_gpu(const int32_t width, const int32_t height)
{
    auto &gpu = mge_context.gpu;

    g_view_logger->info("Creating MGE (D3D11) context with size {}x{}...", width, height);

    MoveWindow(mge_context.hwnd, 0, 0, width, height, TRUE);

    if (!gpu.device)
    {
        create_d3d(mge_context.hwnd);
    }
    else
    {
        gpu.context->OMSetRenderTargets(0, nullptr, nullptr);
        gpu.rtv.Reset();
        const HRESULT hr = gpu.swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        need(hr, "ResizeBuffers");
    }

    ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    need(hr, "GetBuffer");

    hr = gpu.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &gpu.rtv);
    need(hr, "CreateRenderTargetView");

    D3D11_TEXTURE2D_DESC desc;
    back_buffer->GetDesc(&desc);
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(desc.Width);
    viewport.Height = static_cast<float>(desc.Height);
    viewport.MaxDepth = 1.0f;
    gpu.context->RSSetViewports(1, &viewport);

    create_texture(width, height);

    gpu.width = width;
    gpu.height = height;
}

static void apply_requested_size()
{
    need(is_on_gui_thread(), "MGECompositor::apply_requested_size called from non-GUI thread");

    int32_t width{};
    int32_t height{};
    unpack_size(mge_context.requested_size.load(), width, height);
    if (width <= 0 || height <= 0) return;

    std::lock_guard gpu_lock(mge_context.gpu_mutex);
    auto &gpu = mge_context.gpu;

    if (!gpu.device || gpu.width != width || gpu.height != height)
    {
        resize_gpu(width, height);
    }

    std::lock_guard frame_lock(mge_context.frame_mutex);
    upload_and_present();
}

static void present_from_emu_thread()
{
    const auto &frame = mge_context.frame;

    {
        const std::unique_lock gpu_lock(mge_context.gpu_mutex, std::try_to_lock);
        if (gpu_lock.owns_lock() && mge_context.gpu.device && mge_context.gpu.width == frame.width &&
            mge_context.gpu.height == frame.height)
        {
            upload_and_present();
            return;
        }
    }

    const auto size = pack_size(frame.width, frame.height);
    if (size == mge_context.last_requested_size) return;

    mge_context.last_requested_size = size;
    mge_context.requested_size.store(size);
    g_main_ctx.dispatcher->post(apply_requested_size);
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
    case WM_NCDESTROY:
        destroy_d3d();
        break;
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
    int32_t width{};
    int32_t height{};
    PluginUtil::get_video_size(&width, &height);
    if (width <= 0 || height <= 0) return;

    std::lock_guard frame_lock(mge_context.frame_mutex);
    auto &frame = mge_context.frame;

    if (width != frame.width || height != frame.height || !frame.buffer)
    {
        _aligned_free(frame.buffer);

        const auto buffer_size = static_cast<size_t>(width) * height * 4;
        frame.buffer = _aligned_malloc(buffer_size, 16);
        need(frame.buffer, "Failed to allocate MGE buffers");
        ZeroMemory(frame.buffer, buffer_size);

        frame.width = width;
        frame.height = height;
    }

    PluginUtil::read_video(frame.buffer);

    present_from_emu_thread();
}

HWND MGECompositor::hwnd()
{
    return mge_context.hwnd;
}

void MGECompositor::get_video_size(int32_t *width, int32_t *height)
{
    std::lock_guard frame_lock(mge_context.frame_mutex);
    if (width)
    {
        *width = mge_context.frame.width;
    }
    if (height)
    {
        *height = mge_context.frame.height;
    }
}

void MGECompositor::copy_video(void *buffer)
{
    std::lock_guard frame_lock(mge_context.frame_mutex);
    const auto &frame = mge_context.frame;
    if (!frame.buffer) return;
    memcpy(buffer, frame.buffer, static_cast<size_t>(frame.width) * frame.height * 4);
}

void MGECompositor::load_screen(void *data)
{
    std::lock_guard frame_lock(mge_context.frame_mutex);
    const auto &frame = mge_context.frame;
    if (!frame.buffer) return;
    memcpy(frame.buffer, data, static_cast<size_t>(frame.width) * frame.height * 4);

    present_from_emu_thread();
}
