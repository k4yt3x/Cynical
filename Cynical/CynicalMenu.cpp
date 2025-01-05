#include "CynicalMenu.h"

#include <MinHook.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "CynicalUtils.h"

namespace Cynical {
namespace Menu {

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    HRESULT hr = g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr)) {
        DEBUG_PRINT("[!] Failed to get back buffer\n");
        return;
    }

    hr = g_D3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_MainRTV);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        DEBUG_PRINT("[!] Failed to create render target view\n");
    }
}

void CleanupRenderTarget() {
    if (g_MainRTV) {
        g_MainRTV->Release();
        g_MainRTV = nullptr;
    }
}

// Window procedure for the internal overlay window
LRESULT CALLBACK InternalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == VK_F12) {
        g_ImGuiVisible = !g_ImGuiVisible;
        DEBUG_PRINT("[+] Toggled ImGui visibility: %s\n", g_ImGuiVisible ? "ON" : "OFF");
    }

    if (g_ImGuiInitialized) {
        if (g_ImGuiVisible) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        }

        if (g_ImGuiVisible && !g_ImGuiIsInactive) {
            CallWindowProc(g_OriginalWndProc, hWnd, WM_ACTIVATE, WA_INACTIVE, 0);
            g_ImGuiIsInactive = true;
        } else if (!g_ImGuiVisible && g_ImGuiIsInactive) {
            CallWindowProc(g_OriginalWndProc, hWnd, WM_ACTIVATE, WA_ACTIVE, 0);
            g_ImGuiIsInactive = false;
        }
    }

    return g_ImGuiVisible ? g_ImGuiVisible : CallWindowProcW(g_OriginalWndProc, hWnd, uMsg, wParam, lParam);
}

// Window procedure for the external menu window
LRESULT CALLBACK ExternalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_ImGuiInitialized) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return true;
        }
    }

    if (uMsg == WM_DESTROY) {
        // Handle window destruction
        PostQuitMessage(0);
        return 0;
    } else if (uMsg == WM_SIZE) {
        // Handle window resizing
        if (g_D3D11Device != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            HRESULT hr =
                g_SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr)) {
                DEBUG_PRINT("[!] Failed to resize swap chain buffers.\n");
            } else {
                CreateRenderTarget();
            }
        }
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool CreateExternalMenuWindow() {
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        ExternalWndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"Cynical Menu",
        nullptr
    };

    if (!RegisterClassExW(&wc)) {
        DEBUG_PRINT("[!] Failed to register overlay window class.\n");
        return false;
    }

    g_TargetWindow = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Cynical Menu",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,  // Width
        500,  // Height
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!g_TargetWindow) {
        DEBUG_PRINT("[!] Failed to create overlay window.\n");
        return false;
    }

    SetWindowDisplayAffinity(g_TargetWindow, WDA_EXCLUDEFROMCAPTURE);

    return true;
}

bool InitExternalD3D11() {
    DXGI_SWAP_CHAIN_DESC scd{};
    ZeroMemory(&scd, sizeof(scd));
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 160;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = g_TargetWindow;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &scd,
        &g_SwapChain,
        &g_D3D11Device,
        nullptr,
        &g_D3D11DeviceContext
    );

    if (FAILED(hr)) {
        DEBUG_PRINT("[!] Failed to create D3D11 device and swap chain for overlay\n");
        return false;
    }

    // Create render target view
    CreateRenderTarget();

    return true;
}

bool InitImGuiOnceForMode(IDXGISwapChain* pSwapChain = nullptr) {
    DEBUG_PRINT("[+] Initializing ImGui.\n");
    if (g_RenderMode == RenderMode::InternalD3D11) {
        if (!pSwapChain) {
            DEBUG_PRINT("[!] Swap chain not provided for D3D11 mode.\n");
            return false;
        }

        if (!g_D3D11Device || !g_D3D11DeviceContext) {
            if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_D3D11Device))) {
                DEBUG_PRINT("[!] Failed to get D3D11 device from swap chain.\n");
                return false;
            }
            g_D3D11Device->GetImmediateContext(&g_D3D11DeviceContext);
        }

        // Retrieve the window handle from the swap chain
        DXGI_SWAP_CHAIN_DESC desc;
        pSwapChain->GetDesc(&desc);
        g_TargetWindow = desc.OutputWindow;
    }

    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Initialize ImGui
    ImGui_ImplWin32_Init(g_TargetWindow);
    ImGui_ImplDX11_Init(g_D3D11Device, g_D3D11DeviceContext);

    if (g_RenderMode == RenderMode::InternalD3D11) {
        // Hook the window procedure
        g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(g_TargetWindow, GWLP_WNDPROC, (LONG_PTR)InternalWndProc);
        if (!g_OriginalWndProc) {
            DEBUG_PRINT("[!] Failed to hook WndProc! Error: %d\n", GetLastError());
            return false;
        }
    }

    g_ImGuiInitialized = true;
    DEBUG_PRINT("[+] ImGui initialized.\n");
    return true;
}

HRESULT __stdcall HookedD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_ImGuiInitialized) {
        InitImGuiOnceForMode(pSwapChain);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse = g_ImGuiVisible;
    io.WantCaptureKeyboard = g_ImGuiVisible;

    // Render ImGui if initialized and visible
    if (g_ImGuiInitialized && g_ImGuiVisible) {
        // Set the render target to the main render target view
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        if (pBackBuffer) {
            g_D3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_MainRTV);
            pBackBuffer->Release();
        }
        g_D3D11DeviceContext->OMSetRenderTargets(1, &g_MainRTV, nullptr);

        // Start the ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_MenuUpdateCallback) {
            g_MenuUpdateCallback();
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return g_OriginalD3DPresent(pSwapChain, SyncInterval, Flags);
}

// Main render loop for external mode
void RenderExternal() {
    // Create an OS window for the external menu
    if (!CreateExternalMenuWindow()) {
        DEBUG_PRINT("[!] Failed to create external menu window.\n");
        return;
    }
    DEBUG_PRINT("[+] Created external menu window.\n");

    // Initialize D3D11 for the external menu window
    if (!InitExternalD3D11()) {
        DEBUG_PRINT("[!] Failed to initialize D3D11 for the external menu window.\n");
        return;
    }
    DEBUG_PRINT("[+] Initialized D3D11 for the external menu window.\n");

    // Show the external window
    ShowWindow(g_TargetWindow, SW_SHOW);
    UpdateWindow(g_TargetWindow);

    // Initialize ImGui for the external window
    if (!InitImGuiOnceForMode()) {
        DEBUG_PRINT("[!] Failed to initialize ImGui for the external menu window.\n");
        return;
    }

    // Main display loop
    while (g_OverlayActive) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                g_OverlayActive = false;
            }
        }
        if (!g_OverlayActive) {
            break;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_MenuUpdateCallback) {
            g_MenuUpdateCallback();
        }

        ImGui::Render();

        constexpr float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        g_D3D11DeviceContext->OMSetRenderTargets(1, &g_MainRTV, nullptr);
        g_D3D11DeviceContext->ClearRenderTargetView(g_MainRTV, clearColor);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_SwapChain->Present(1, 0);
    }
}

// Create a Dummy Device to find the Present vtable pointer, then hook it
bool Init() {
    if (g_RenderMode == RenderMode::InternalD3D11) {
        // Initialize MinHook
        if (MH_Initialize() != MH_OK) {
            DEBUG_PRINT("[!] MinHook initialization failed.\n");
            return TRUE;
        }
        DEBUG_PRINT("[+] MinHook initialized.\n");

        // Set the default menu update callback
        if (!g_MenuUpdateCallback) {
            SetMenuUpdateCallback(&ImGui::ShowDemoWindow);
        }

        // Get the address of the Direct3D 11 Present function
        void* pPresentAddr = Utils::D3D11::GetPresentAddress();

        // Create a hook for Present
        if (MH_CreateHook(pPresentAddr, &HookedD3D11Present, reinterpret_cast<LPVOID*>(&g_OriginalD3DPresent)) !=
            MH_OK) {
            DEBUG_PRINT("[!] MH_CreateHook for Present failed.\n");
            return false;
        }

        // Enable the hook for Present
        if (MH_EnableHook(pPresentAddr) != MH_OK) {
            DEBUG_PRINT("[!] Failed to enable Present hook.\n");
        }
        DEBUG_PRINT("[+] Present hook created and enabled.\n");
    } else if (g_RenderMode == RenderMode::ExternalD3D11) {
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(RenderExternal), nullptr, 0, nullptr);
        DEBUG_PRINT("[+] Created external present loop thread.\n");
    } else {
        DEBUG_PRINT("[!] Invalid render mode specified.\n");
        return false;
    }

    return true;
}

void Destroy() {
    if (g_ImGuiInitialized) {
        DEBUG_PRINT("[+] Shutting down ImGui.\n");
        SetWindowLongPtrW(g_TargetWindow, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_ImGuiInitialized = false;
    }

    if (g_RenderMode == RenderMode::InternalD3D11) {
        // Disable all hooks and uninitialize MinHook
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();

    } else if (g_RenderMode == RenderMode::ExternalD3D11) {
        if (g_TargetWindow) {
            DestroyWindow(g_TargetWindow);
            UnregisterClassW(L"CynicalMenu", GetModuleHandle(nullptr));
        }
    }

    // Release any stored real device/context references
    if (g_D3D11DeviceContext) {
        g_D3D11DeviceContext->Release();
        g_D3D11DeviceContext = nullptr;
    }
    if (g_D3D11Device) {
        g_D3D11Device->Release();
        g_D3D11Device = nullptr;
    }
}

void SetMenuUpdateCallback(void* pMenuUpdateCallback) {
    g_MenuUpdateCallback = reinterpret_cast<void (*)()>(pMenuUpdateCallback);
}

void SetRenderMode(RenderMode mode) {
    g_RenderMode = mode;
}

}  // namespace Menu
}  // namespace Cynical