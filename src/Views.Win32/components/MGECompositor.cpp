// MGECompositor_D3D11.cpp
// Replace your GDI-based compositor with this D3D11-backed implementation.
//
// Build: link d3d11.lib dxgi.lib d3dcompiler.lib
// (Add appropriate include/library paths if necessary.)

#include "stdafx.h"
#include "MGECompositor.h"
#include <Messenger.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

constexpr auto CONTROL_CLASS_NAME = L"game_control";
constexpr DXGI_FORMAT TEXTURE_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;

struct t_mge_context
{
    int32_t last_width{};
    int32_t last_height{};
    int32_t width{};
    int32_t height{};
    void *buffer{};

    HWND hwnd{};

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
};

static t_mge_context mge_context{};

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_SIZE:
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void log_hr(const char *what, HRESULT hr)
{
    if (FAILED(hr) && g_view_logger)
    {
        g_view_logger->error("{} failed: 0x{:08X}", what, static_cast<unsigned>(hr));
    }
}

static bool create_d3d_for_hwnd(HWND target_hwnd)
{
    DXGI_SWAP_CHAIN_DESC scdesc = {};
    scdesc.BufferDesc.Width = 0;
    scdesc.BufferDesc.Height = 0;
    scdesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scdesc.SampleDesc.Count = 1;
    scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scdesc.BufferCount = 1;
    scdesc.OutputWindow = target_hwnd;
    scdesc.Windowed = TRUE;
    scdesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scdesc.Flags = 0;

    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};

    ID3D11Device *device_raw{};
    ID3D11DeviceContext *context_raw{};
    IDXGISwapChain *swap_raw{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, feature_levels,
                                               ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &scdesc, &swap_raw,
                                               &device_raw, nullptr, &context_raw);

    if (FAILED(hr))
    {
        log_hr("D3D11CreateDeviceAndSwapChain", hr);
        return false;
    }

    mge_context.device.Attach(device_raw);
    mge_context.context.Attach(context_raw);
    mge_context.swapchain.Attach(swap_raw);

    // create RTV for swapchain back buffer
    ComPtr<ID3D11Texture2D> back_buffer;
    hr = mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (FAILED(hr))
    {
        log_hr("GetBuffer", hr);
        return false;
    }

    hr = mge_context.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &mge_context.rtv);
    if (FAILED(hr))
    {
        log_hr("CreateRenderTargetView", hr);
        return false;
    }

    // Point sampler for nearest-neighbour scaling
    D3D11_SAMPLER_DESC sampdesc = {};
    sampdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampdesc.MinLOD = 0;
    sampdesc.MaxLOD = D3D11_FLOAT32_MAX;
    mge_context.device->CreateSamplerState(&sampdesc, &mge_context.sampler);

    // Compile minimal shaders (vertex shader using SV_VertexID to generate a fullscreen quad)
    const auto vs_src = R"(
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

    const auto ps_src = R"(
    Texture2D tex : register(t0);
    SamplerState samp : register(s0);
    struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
    float4 main(PSIn input) : SV_TARGET
    {
        float4 c = tex.Sample(samp, input.uv);
        return c;
    }
    )";

    ComPtr<ID3DBlob> vs_blob, ps_blob, err_blob;
    hr = D3DCompile(vs_src, strlen(vs_src), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vs_blob, &err_blob);
    if (FAILED(hr))
    {
        if (err_blob) g_view_logger->error("VS compile: {}", static_cast<const char *>(err_blob->GetBufferPointer()));
        log_hr("D3DCompile VS", hr);
        return false;
    }
    hr = D3DCompile(ps_src, strlen(ps_src), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &ps_blob, &err_blob);
    if (FAILED(hr))
    {
        if (err_blob) g_view_logger->error("PS compile: {}", static_cast<const char *>(err_blob->GetBufferPointer()));
        log_hr("D3DCompile PS", hr);
        return false;
    }

    hr = mge_context.device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr,
                                                &mge_context.vs);
    if (FAILED(hr))
    {
        log_hr("CreateVertexShader", hr);
        return false;
    }
    hr = mge_context.device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr,
                                               &mge_context.ps);
    if (FAILED(hr))
    {
        log_hr("CreatePixelShader", hr);
        return false;
    }

    return true;
}

static void destroy_d3d_resources()
{
    mge_context.srv.Reset();
    mge_context.texture.Reset();
    mge_context.rtv.Reset();
    if (mge_context.swapchain)
    {
        mge_context.swapchain->SetFullscreenState(FALSE, nullptr);
    }
    mge_context.swapchain.Reset();
    mge_context.context.Reset();
    mge_context.device.Reset();
    mge_context.vs.Reset();
    mge_context.ps.Reset();
    mge_context.sampler.Reset();
}

static bool ensure_texture_exists_with_size(const int w, const int h)
{
    if (!mge_context.device) return false;
    if (mge_context.texture && mge_context.last_width == w && mge_context.last_height == h) return true;

    mge_context.srv.Reset();
    mge_context.texture.Reset();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = (UINT)w;
    desc.Height = (UINT)h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = TEXTURE_FORMAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    HRESULT hr = mge_context.device->CreateTexture2D(&desc, nullptr, &mge_context.texture);
    if (FAILED(hr))
    {
        log_hr("CreateTexture2D", hr);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = desc.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    hr = mge_context.device->CreateShaderResourceView(mge_context.texture.Get(), &srvd, &mge_context.srv);
    if (FAILED(hr))
    {
        log_hr("CreateShaderResourceView", hr);
        return false;
    }

    mge_context.last_width = w;
    mge_context.last_height = h;
    return true;
}

static void upload_frame_to_texture(const void *src_buffer, const int src_width, const int src_height)
{
    if (!mge_context.texture) return;
    D3D11_MAPPED_SUBRESOURCE mapped;
    const HRESULT hr = mge_context.context->Map(mge_context.texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
    {
        log_hr("Map texture", hr);
        return;
    }

    const auto src = static_cast<const uint8_t *>(src_buffer);
    const auto dst = static_cast<uint8_t *>(mapped.pData);
    const int dst_pitch = static_cast<int>(mapped.RowPitch);

    constexpr int src_bpp = 3;
    for (int y = 0; y < src_height; ++y)
    {
        const uint8_t *srow = src + y * (src_width * src_bpp);
        uint8_t *drow = dst + y * dst_pitch;
        for (int x = 0; x < src_width; ++x)
        {
            drow[x * 4 + 0] = srow[x * src_bpp + 0];
            drow[x * 4 + 1] = srow[x * src_bpp + 1];
            drow[x * 4 + 2] = srow[x * src_bpp + 2];
            drow[x * 4 + 3] = 0xFF;
        }
    }

    mge_context.context->Unmap(mge_context.texture.Get(), 0);
}

// Render the texture to the back buffer and present
static void render_and_present()
{
    if (!mge_context.context || !mge_context.rtv || !mge_context.srv) return;

    // Set render target
    ID3D11RenderTargetView *rtv = mge_context.rtv.Get();
    mge_context.context->OMSetRenderTargets(1, &rtv, nullptr);

    // Set viewport to the swapchain size (query back buffer desc)
    ComPtr<ID3D11Texture2D> bb;
    if (SUCCEEDED(mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&bb))))
    {
        D3D11_TEXTURE2D_DESC bbdesc;
        bb->GetDesc(&bbdesc);
        D3D11_VIEWPORT vp;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.Width = (float)bbdesc.Width;
        vp.Height = (float)bbdesc.Height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        mge_context.context->RSSetViewports(1, &vp);
    }

    // Clear (optional)
    float clearColor[4] = {0, 0, 0, 1};
    mge_context.context->ClearRenderTargetView(mge_context.rtv.Get(), clearColor);

    // Set pipeline
    mge_context.context->VSSetShader(mge_context.vs.Get(), nullptr, 0);
    mge_context.context->PSSetShader(mge_context.ps.Get(), nullptr, 0);
    ID3D11ShaderResourceView *srvs[] = {mge_context.srv.Get()};
    mge_context.context->PSSetShaderResources(0, 1, srvs);
    ID3D11SamplerState *samps[] = {mge_context.sampler.Get()};
    mge_context.context->PSSetSamplers(0, 1, samps);

    mge_context.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw 6 vertices, no vertex buffer (VS uses SV_VertexID)
    mge_context.context->Draw(6, 0);

    HRESULT hr = mge_context.swapchain->Present(0, 0);
    if (FAILED(hr)) log_hr("Present", hr);

    // Unbind SRV to allow mapping texture next frame on some drivers
    ID3D11ShaderResourceView *nullSRV[1] = {nullptr};
    mge_context.context->PSSetShaderResources(0, 1, nullSRV);
}

static void recreate_mge_context_d3d()
{
    g_view_logger->info("Creating MGE (D3D11) context with size {}x{}...", mge_context.width, mge_context.height);

    if (!mge_context.device && !create_d3d_for_hwnd(mge_context.hwnd))
    {
        g_view_logger->error("Failed to create D3D device/swapchain for MGE compositor.");
        return;
    }

    free(mge_context.buffer);
    mge_context.buffer = malloc(mge_context.width * mge_context.height * 3);

    RECT rc;
    GetClientRect(mge_context.hwnd, &rc);
    const UINT w = static_cast<UINT>(rc.right - rc.left);
    const UINT h = static_cast<UINT>(rc.bottom - rc.top);

    if (mge_context.swapchain)
    {
        mge_context.rtv.Reset();
        HRESULT hr = mge_context.swapchain->ResizeBuffers(1, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        if (FAILED(hr)) log_hr("ResizeBuffers", hr);

        ComPtr<ID3D11Texture2D> backBuffer;
        hr = mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (SUCCEEDED(hr))
        {
            hr = mge_context.device->CreateRenderTargetView(backBuffer.Get(), nullptr, &mge_context.rtv);
            if (FAILED(hr)) log_hr("CreateRenderTargetView (after resize)", hr);
        }
    }

    ensure_texture_exists_with_size(mge_context.width, mge_context.height);
}

void MGECompositor::create(HWND hwnd)
{
    mge_context.hwnd = CreateWindow(CONTROL_CLASS_NAME, L"", WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, nullptr,
                                    g_main_ctx.hinst, nullptr);
}

void MGECompositor::init()
{
    WNDCLASS wndclass = {0};
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = (WNDPROC)wndproc;
    wndclass.hInstance = g_main_ctx.hinst;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = CONTROL_CLASS_NAME;
    RegisterClass(&wndclass);

    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, [](const std::any &data) {
        const auto value = std::any_cast<bool>(data);
        ShowWindow(mge_context.hwnd, value && g_main_ctx.core_ctx->vr_get_mge_available() ? SW_SHOW : SW_HIDE);
    });
}

void MGECompositor::update_screen()
{
    g_main_ctx.core.plugin_funcs.video_get_video_size(&mge_context.width, &mge_context.height);

    if (mge_context.width != mge_context.last_width || mge_context.height != mge_context.last_height)
    {
        MoveWindow(mge_context.hwnd, 0, 0, mge_context.width, mge_context.height, TRUE);

        recreate_mge_context_d3d();
    }

    g_main_ctx.core.plugin_funcs.video_read_video(&mge_context.buffer);

    if (!ensure_texture_exists_with_size(mge_context.width, mge_context.height))
    {
        return;
    }

    upload_frame_to_texture(mge_context.buffer, mge_context.width, mge_context.height);

    render_and_present();

    mge_context.last_width = mge_context.width;
    mge_context.last_height = mge_context.height;
}

void MGECompositor::get_video_size(int32_t *width, int32_t *height)
{
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
    memcpy(buffer, mge_context.buffer, mge_context.width * mge_context.height * 3);
}

void MGECompositor::load_screen(void *data)
{
    memcpy(mge_context.buffer, data, mge_context.width * mge_context.height * 3);

    if (ensure_texture_exists_with_size(mge_context.width, mge_context.height))
    {
        upload_frame_to_texture(mge_context.buffer, mge_context.width, mge_context.height);
        render_and_present();
    }
}
