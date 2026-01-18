#include "vendor.hpp"

#ifndef IDA_RE_PLATFORM_WINDOWS
#error "This application currently only supports Windows (DirectX 11). Linux/macOS support requires OpenGL backend."
#endif

#include "api/llm_api.hpp"
#include "api/mcp_client.hpp"
#include "core/config.hpp"
#include "ui/ui.hpp"

#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

static ID3D11Device           *g_pd3dDevice           = nullptr;
static ID3D11DeviceContext    *g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain         *g_pSwapChain           = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

bool           CreateDeviceD3D( HWND hWnd );
void           CleanupDeviceD3D( );
void           CreateRenderTarget( );
void           CleanupRenderTarget( );
LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

void InitializeImGuiFonts( ) {
    ImGuiIO &io = ImGui::GetIO( );

    // Clear default font to load custom fonts
    io.Fonts->Clear( );

    // Try to load system fonts or fallback to ImGui default
    ImFontConfig config;
    config.SizePixels  = 13.0f;
    config.OversampleH = 3;
    config.OversampleV = 3;
    config.PixelSnapH  = true;

    // Load main UI font (Segoe UI or fallback)
    const char *font_path = "C:\\Windows\\Fonts\\Verdana.ttf";
    if ( std::filesystem::exists( font_path ) ) {
        io.Fonts->AddFontFromFileTTF( font_path, 15.0f, &config );
    } else {
        // Fallback to default font
        io.Fonts->AddFontDefault( );
    }

    // Rebuild font texture
    io.Fonts->Build( );
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int ) {
    WNDCLASSEXW wc
        = { sizeof( wc ), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"IDA RE Assistant", nullptr };
    RegisterClassExW( &wc );

    HWND hwnd = CreateWindowW( wc.lpszClassName, L"IDA RE Assistant", WS_OVERLAPPEDWINDOW, 100, 100, 1400, 900, nullptr, nullptr, hInstance,
                               nullptr );

    if ( !CreateDeviceD3D( hwnd ) ) {
        CleanupDeviceD3D( );
        UnregisterClassW( wc.lpszClassName, hInstance );
        return 1;
    }

    ShowWindow( hwnd, SW_SHOWDEFAULT );
    UpdateWindow( hwnd );

    IMGUI_CHECKVERSION( );
    ImGui::CreateContext( );
    ImGuiIO &io     = ImGui::GetIO( );
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init( hwnd );
    ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );

    // Initialize fonts and icons AFTER setting up rendering backend
    InitializeImGuiFonts( );

    // init components
    ida_re::core::app_config_t config;
    config.load( );

    ida_re::api::c_mcp_client mcp;
    mcp.set_host( config.m_mcp_host );
    mcp.set_port( config.m_mcp_port );

    ida_re::api::c_llm_manager llm_manager;

    // apply config to LLM manager
    if ( config.m_provider == "claude" ) {
        llm_manager.set_provider( ida_re::api::e_provider::claude );
        llm_manager.claude( ).set_api_key( config.m_claude_api_key );
        llm_manager.claude( ).set_model( config.m_model );
        llm_manager.claude( ).set_max_tokens( config.m_max_tokens );
    } else if ( config.m_provider == "openai" ) {
        llm_manager.set_provider( ida_re::api::e_provider::openai );
        llm_manager.openai( ).set_api_key( config.m_openai_api_key );
        llm_manager.openai( ).set_model( config.m_model );
        llm_manager.openai( ).set_max_tokens( config.m_max_tokens );
    } else {
        llm_manager.set_provider( ida_re::api::e_provider::gemini );
        llm_manager.gemini( ).set_api_key( config.m_gemini_api_key );
        llm_manager.gemini( ).set_model( config.m_model );
        llm_manager.gemini( ).set_max_tokens( config.m_max_tokens );
    }

    ida_re::ui::c_ui ui;
    ui.set_mcp_client( &mcp );
    ui.set_llm_manager( &llm_manager );
    ui.set_config( &config );
    ui.init( );

    // auto connect if configured
    if ( config.m_auto_connect ) {
        [[maybe_unused]] const auto connected = mcp.connect( );
    }

    ImVec4 clear_color = ImVec4( 0.03f, 0.02f, 0.05f, 1.00f );

    bool running = true;
    while ( running ) {
        MSG msg;
        while ( PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
            if ( msg.message == WM_QUIT ) {
                running = false;
            }
        }

        if ( !running )
            break;

        ImGui_ImplDX11_NewFrame( );
        ImGui_ImplWin32_NewFrame( );
        ImGui::NewFrame( );

        ui.render( );

        ImGui::Render( );
        const float clear_color_with_alpha[ 4 ]
            = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets( 1, &g_mainRenderTargetView, nullptr );
        g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView, clear_color_with_alpha );
        ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

        g_pSwapChain->Present( 1, 0 ); // vsync
    }

    ui.shutdown( );
    config.save( );

    // Shutdown
    ImGui_ImplDX11_Shutdown( );
    ImGui_ImplWin32_Shutdown( );
    ImGui::DestroyContext( );

    CleanupDeviceD3D( );
    DestroyWindow( hwnd );
    UnregisterClassW( wc.lpszClassName, hInstance );

    return 0;
}

bool CreateDeviceD3D( HWND hWnd ) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT                    createDeviceFlags = 0;
    D3D_FEATURE_LEVEL       featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[ 2 ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res
        = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
                                         D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext );

    if ( res == DXGI_ERROR_UNSUPPORTED ) {
        res = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2,
                                             D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext );
    }

    if ( res != S_OK )
        return false;

    CreateRenderTarget( );
    return true;
}

void CleanupDeviceD3D( ) {
    CleanupRenderTarget( );
    if ( g_pSwapChain ) {
        g_pSwapChain->Release( );
        g_pSwapChain = nullptr;
    }
    if ( g_pd3dDeviceContext ) {
        g_pd3dDeviceContext->Release( );
        g_pd3dDeviceContext = nullptr;
    }
    if ( g_pd3dDevice ) {
        g_pd3dDevice->Release( );
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget( ) {
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
    g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_mainRenderTargetView );
    pBackBuffer->Release( );
}

void CleanupRenderTarget( ) {
    if ( g_mainRenderTargetView ) {
        g_mainRenderTargetView->Release( );
        g_mainRenderTargetView = nullptr;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
    if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
        return true;

    switch ( msg ) {
        case WM_SIZE :
            if ( g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED ) {
                CleanupRenderTarget( );
                g_pSwapChain->ResizeBuffers( 0, ( UINT ) LOWORD( lParam ), ( UINT ) HIWORD( lParam ), DXGI_FORMAT_UNKNOWN, 0 );
                CreateRenderTarget( );
            }
            return 0;
        case WM_SYSCOMMAND :
            if ( ( wParam & 0xfff0 ) == SC_KEYMENU )
                return 0;
            break;
        case WM_DESTROY :
            PostQuitMessage( 0 );
            return 0;
    }
    return DefWindowProcW( hWnd, msg, wParam, lParam );
}
