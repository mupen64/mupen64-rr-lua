// MGECompositor_D3D11.cpp
// Replace your GDI-based compositor with this D3D11-backed implementation.
//
// Build: link d3d11.lib dxgi.lib d3dcompiler.lib
// (Add appropriate include/library paths if necessary.)

#include "stdafx.h"
#include <components/MGECompositor.h>
#include <Plugin.h>
#include <Messenger.h>

using Microsoft::WRL::ComPtr;
constexpr auto CONTROL_CLASS_NAME = L"game_control";
constexpr DXGI_FORMAT TEXTURE_FORMAT = DXGI_FORMAT_B8G8R8X8_UNORM;
constexpr float CLEAR_COLOR[4] = {0, 0, 0, 1};

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

struct t_mge_context
{
    int32_t last_width{};
    int32_t last_height{};
    int32_t width{};
    int32_t height{};
    void *buffer{};
    void *rgba_buffer{};

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

static void create_d3d(const HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC scdesc = {};
    scdesc.BufferDesc.Width = 0;
    scdesc.BufferDesc.Height = 0;
    scdesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scdesc.SampleDesc.Count = 1;
    scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scdesc.BufferCount = 1;
    scdesc.OutputWindow = hwnd;
    scdesc.Windowed = TRUE;
    scdesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scdesc.Flags = 0;

    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};

    ID3D11Device *device_raw{};
    ID3D11DeviceContext *context_raw{};
    IDXGISwapChain *swap_raw{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, feature_levels,
                                               ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &scdesc, &swap_raw,
                                               &device_raw, nullptr, &context_raw);
    RT_ASSERT_HR(hr, L"D3D11CreateDeviceAndSwapChain");

    mge_context.device.Attach(device_raw);
    mge_context.context.Attach(context_raw);
    mge_context.swapchain.Attach(swap_raw);

    // create RTV for swapchain back buffer
    ComPtr<ID3D11Texture2D> back_buffer;
    hr = mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    RT_ASSERT_HR(hr, L"GetBuffer");

    hr = mge_context.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &mge_context.rtv);
    RT_ASSERT_HR(hr, L"CreateRenderTargetView");

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
    RT_ASSERT_HR(hr, L"CreateSamplerState");

    ComPtr<ID3DBlob> vs_blob, ps_blob, err_blob;
    hr = D3DCompile(VERTEX_SHADER.data(), VERTEX_SHADER.size(), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0,
                    &vs_blob, &err_blob);
    RT_ASSERT_HR(hr, L"D3DCompile");

    hr = D3DCompile(FRAGMENT_SHADER.data(), FRAGMENT_SHADER.size(), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0,
                    &ps_blob, &err_blob);
    RT_ASSERT_HR(hr, L"D3DCompile");

    hr = mge_context.device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr,
                                                &mge_context.vs);
    RT_ASSERT_HR(hr, L"CreateVertexShader");

    hr = mge_context.device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr,
                                               &mge_context.ps);
    RT_ASSERT_HR(hr, L"CreatePixelShader");

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
    desc.Format = TEXTURE_FORMAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    HRESULT hr = mge_context.device->CreateTexture2D(&desc, nullptr, &mge_context.texture);
    RT_ASSERT_HR(hr, L"CreateTexture2D");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = desc.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    hr = mge_context.device->CreateShaderResourceView(mge_context.texture.Get(), &srvd, &mge_context.srv);
    RT_ASSERT_HR(hr, L"CreateShaderResourceView");

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

    mge_context.context->UpdateSubresource(mge_context.texture.Get(), 0, &db, mge_context.rgba_buffer,
                                           mge_context.width * 4, 0);
}

static void render_and_present()
{
    RT_ASSERT(mge_context.context && mge_context.rtv && mge_context.srv, L"D3D context not initialized");

    ID3D11RenderTargetView *rtv = mge_context.rtv.Get();
    mge_context.context->OMSetRenderTargets(1, &rtv, nullptr);

    ComPtr<ID3D11Texture2D> bb;
    HRESULT hr = mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&bb));
    RT_ASSERT_HR(hr, L"GetBuffer");

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

    mge_context.context->ClearRenderTargetView(mge_context.rtv.Get(), CLEAR_COLOR);

    // Bind SRV
    ID3D11ShaderResourceView *srvs[] = {mge_context.srv.Get()};
    mge_context.context->PSSetShaderResources(0, 1, srvs);

    // Draw fullscreen quad
    mge_context.context->Draw(6, 0);

    // Unbind SRV to allow UpdateSubresource
    ID3D11ShaderResourceView *null_srv[1] = {nullptr};
    mge_context.context->PSSetShaderResources(0, 1, null_srv);

    hr = mge_context.swapchain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
}

static void copy_rgb24_buffer_to_rgb32()
{
    const auto src = static_cast<const uint8_t *>(mge_context.buffer);
    const auto dst = static_cast<uint8_t *>(mge_context.rgba_buffer);

    const int width = mge_context.width;
    const int height = mge_context.height;

    for (int y = 0; y < height; ++y)
    {
        const uint8_t *srow = src + y * width * 3;
        uint8_t *drow = dst + y * width * 4;

        for (int x = 0; x < width; ++x)
        {
            std::memcpy(drow, srow, 3);

            srow += 3;
            drow += 4;
        }
    }
}

static void recreate_mge_context_d3d()
{
    g_view_logger->info("Creating MGE (D3D11) context with size {}x{}...", mge_context.width, mge_context.height);

    if (!mge_context.device)
    {
        create_d3d(mge_context.hwnd);
    }

    _aligned_free(mge_context.buffer);
    _aligned_free(mge_context.rgba_buffer);

    const auto rgb24_buffer_size = mge_context.width * mge_context.height * 3;
    const auto rgba32_buffer_size = mge_context.width * mge_context.height * 4;

    mge_context.buffer = _aligned_malloc(rgb24_buffer_size, 16);
    mge_context.rgba_buffer = _aligned_malloc(rgba32_buffer_size, 16);
    RT_ASSERT(mge_context.buffer && mge_context.rgba_buffer, L"Failed to allocate MGE buffers");

    ZeroMemory(mge_context.buffer, rgb24_buffer_size);
    ZeroMemory(mge_context.rgba_buffer, rgba32_buffer_size);

    RECT rc;
    RT_ASSERT(GetClientRect(mge_context.hwnd, &rc), L"GetClientRect failed");
    const UINT w = static_cast<UINT>(rc.right - rc.left);
    const UINT h = static_cast<UINT>(rc.bottom - rc.top);

    if (mge_context.swapchain)
    {
        mge_context.rtv.Reset();
        HRESULT hr = mge_context.swapchain->ResizeBuffers(1, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        RT_ASSERT_HR(hr, L"ResizeBuffers");

        ComPtr<ID3D11Texture2D> backBuffer;
        hr = mge_context.swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        RT_ASSERT_HR(hr, L"GetBuffer");

        hr = mge_context.device->CreateRenderTargetView(backBuffer.Get(), nullptr, &mge_context.rtv);
        RT_ASSERT_HR(hr, L"CreateRenderTargetView");
    }

    ensure_texture_exists_with_size(mge_context.width, mge_context.height);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
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
        ShowWindow(mge_context.hwnd, value && PluginUtil::mge_available() ? SW_SHOW : SW_HIDE);
    });
}

void MGECompositor::update_screen()
{
    g_plugin_funcs.video_get_video_size(&mge_context.width, &mge_context.height);

    if (mge_context.width != mge_context.last_width || mge_context.height != mge_context.last_height)
    {
        MoveWindow(mge_context.hwnd, 0, 0, mge_context.width, mge_context.height, TRUE);
        recreate_mge_context_d3d();
    }

    g_plugin_funcs.video_read_video(&mge_context.buffer);

    copy_rgb24_buffer_to_rgb32();
    upload_rgb32_buffer();
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

    copy_rgb24_buffer_to_rgb32();
    ensure_texture_exists_with_size(mge_context.width, mge_context.height);
    upload_rgb32_buffer();
    render_and_present();
}
