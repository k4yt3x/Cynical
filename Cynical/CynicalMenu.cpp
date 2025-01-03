#include "CynicalMenu.h"

#include <MinHook.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "CynicalUtils.h"

namespace Cynical {
namespace Menu {

// Minimal Win32 message handler (for ImGui input, if the game forwards messages)
LRESULT CALLBACK ImGuiWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

// Initialize ImGui once we have a real device / swapchain
void InitImGuiOnce(IDXGISwapChain* pSwapChain) {
    if (!pSwapChain) {
        return;
    }

    // If we haven't stored the device/context yet, try to retrieve them
    if (!g_D3D11Device || !g_D3D11DeviceContext) {
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_D3D11Device))) {
            return;
        }
        g_D3D11Device->GetImmediateContext(&g_D3D11DeviceContext);
    }

    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Retrieve the window handle from the swap chain
    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);
    g_HWnd = desc.OutputWindow;

    // Initialize ImGui
    ImGui_ImplWin32_Init(g_HWnd);
    ImGui_ImplDX11_Init(g_D3D11Device, g_D3D11DeviceContext);

    // Hook the window procedure
    g_OriginalWndProc = (WNDPROC)SetWindowLongPtrW(g_HWnd, GWLP_WNDPROC, (LONG_PTR)ImGuiWndProc);
    if (!g_OriginalWndProc) {
        DEBUG_PRINT("[!] Failed to hook WndProc! Error: %d\n", GetLastError());
    }
}

// Destroy ImGui
void DestroyImGui() {
    DEBUG_PRINT("[+] Shutting down ImGui.\n");
    SetWindowLongPtr(g_HWnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_ImGuiInitialized = false;
}

// Hooked Present
HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    // Now perform ImGui rendering
    if (!g_ImGuiInitialized) {
        // Initialize ImGui on the first Present call
        InitImGuiOnce(pSwapChain);
        g_ImGuiInitialized = true;
        DEBUG_PRINT("[+] ImGui initialized.\n");
    }

    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse = g_ImGuiVisible;
    io.WantCaptureKeyboard = g_ImGuiVisible;

    // Render ImGui if initialized
    if (g_ImGuiInitialized && g_ImGuiVisible) {
        // Setup ImGui render target
        ID3D11RenderTargetView* pRTV = nullptr;
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        if (pBackBuffer) {
            g_D3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &pRTV);
            pBackBuffer->Release();
        }
        g_D3D11DeviceContext->OMSetRenderTargets(1, &pRTV, nullptr);

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

// Create a Dummy Device to find the Present vtable pointer, then hook it
bool Init() {
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
    if (MH_CreateHook(pPresentAddr, &HookedPresent, reinterpret_cast<LPVOID*>(&g_OriginalD3DPresent)) != MH_OK) {
        DEBUG_PRINT("[!] MH_CreateHook for Present failed.\n");
        return false;
    }

    // Enable the hook for Present
    if (MH_EnableHook(pPresentAddr) != MH_OK) {
        DEBUG_PRINT("[!] Failed to enable Present hook.\n");
    }

    DEBUG_PRINT("[+] Present hook created and enabled.\n");
    return true;
}

void Destroy() {
    if (g_ImGuiInitialized) {
        DestroyImGui();
    }

    // Disable all hooks and uninitialize MinHook
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    // Release any real device/context references if we stored them
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

}  // namespace Menu
}  // namespace Cynical