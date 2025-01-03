#pragma once

#include <d3d11.h>
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Cynical {
namespace Menu {

// Type definitions
typedef HRESULT(__stdcall* D3DPresentFn)(IDXGISwapChain*, UINT, UINT);

// ImGui globals
static bool g_ImGuiInitialized = false;
static bool g_ImGuiIsInactive = false;
static bool g_ImGuiVisible = false;
static void (*g_MenuUpdateCallback)() = nullptr;

// D3D11 globals
static ID3D11Device* g_D3D11Device = nullptr;
static ID3D11DeviceContext* g_D3D11DeviceContext = nullptr;

// DXGI globals
static D3DPresentFn g_OriginalD3DPresent = nullptr;

// Win32 globals
static HWND g_HWnd = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;

bool Init();
void Destroy();
void SetMenuUpdateCallback(void* pSwapChain);

}  // namespace Menu
}  // namespace Cynical