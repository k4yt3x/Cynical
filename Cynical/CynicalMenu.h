#pragma once

#include <d3d11.h>
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(__stdcall* DXGIPresentFn)(IDXGISwapChain*, UINT, UINT);

namespace Cynical {
namespace Menu {

enum class RenderMode {
    InternalD3D11,
    ExternalD3D11
};

// Menu options
static void (*g_MenuUpdateCallback)() = nullptr;
static RenderMode g_RenderMode = RenderMode::InternalD3D11;

// Win32 globals
static HWND g_TargetWindow = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;

// DXGI globals
static DXGIPresentFn g_OriginalDXGIPresent = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;

// D3D11 globals
static ID3D11Device* g_D3D11Device = nullptr;
static ID3D11DeviceContext* g_D3D11DeviceContext = nullptr;
static ID3D11RenderTargetView* g_RTV = nullptr;

// ImGui globals
static bool g_ImGuiInitialized = false;
static bool g_ImGuiIsInactive = false;
static bool g_ImGuiVisible = false;

bool Init();
void Destroy();
void SetMenuUpdateCallback(void* callback);
void SetRenderMode(RenderMode mode);

}  // namespace Menu
}  // namespace Cynical