#pragma once

#include <d3d11.h>
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(__stdcall* D3DPresentFn)(IDXGISwapChain*, UINT, UINT);

namespace Cynical {
namespace Menu {

enum class RenderMode {
    InternalD3D11,
    ExternalD3D11
};

// Menu options
static void (*g_MenuUpdateCallback)() = nullptr;
static RenderMode g_RenderMode = RenderMode::InternalD3D11;

// ImGui globals
static bool g_ImGuiInitialized = false;
static bool g_ImGuiIsInactive = false;
static bool g_ImGuiVisible = false;

// DXGI globals
static D3DPresentFn g_OriginalD3DPresent = nullptr;

// Win32 globals
static HWND g_TargetWindow = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;

// D3D11 globals
static ID3D11Device* g_D3D11Device = nullptr;
static ID3D11DeviceContext* g_D3D11DeviceContext = nullptr;
static ID3D11RenderTargetView* g_MainRTV = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;

// External window globals
static bool g_OverlayActive = true;
static ID3D11BlendState* g_BlendState = nullptr;

bool Init();
void Destroy();
void SetMenuUpdateCallback(void* callback);
void SetRenderMode(RenderMode mode);

}  // namespace Menu
}  // namespace Cynical