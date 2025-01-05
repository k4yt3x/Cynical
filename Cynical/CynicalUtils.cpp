#include "CynicalUtils.h"

#include <Windows.h>
#include <d3d11.h>

namespace Cynical {
namespace Utils {

namespace Memory {

__int64 ResolveAddress(__int64 baseAddress, std::initializer_list<__int64> offsets) {
    __int64 address = baseAddress;
    for (auto offset : offsets) {
        address = *reinterpret_cast<__int64*>(address);
        address += offset;
    }
    return address;
}

template <typename T>
void Write(__int64 address, T value) {
    DWORD oldProtection;
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtection);
    *reinterpret_cast<T*>(address) = value;
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtection, &oldProtection);
}

template <typename K>
K Read(__int64 address) {
    return *reinterpret_cast<K*>(address);
}

void WriteBytes(__int64 address, const std::vector<BYTE>& bytes) {
    DWORD oldProtection;
    VirtualProtect(reinterpret_cast<void*>(address), bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtection);
    memcpy(reinterpret_cast<void*>(address), bytes.data(), bytes.size());
    VirtualProtect(reinterpret_cast<void*>(address), bytes.size(), oldProtection, &oldProtection);
}

std::vector<BYTE> ReadBytes(__int64 address, SIZE_T numBytes) {
    std::vector<BYTE> bytes(numBytes);
    memcpy(bytes.data(), reinterpret_cast<void*>(address), numBytes);
    return bytes;
}

}  // namespace Memory

namespace D3D11 {

void* GetPresentAddress() {
    // Create a hidden window for the dummy swap chain
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        DefWindowProc,
        0L,
        0L,
        GetModuleHandleW(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"DummyWindow",
        nullptr
    };
    RegisterClassExW(&wc);

    // Create a dummy window
    HWND hDummyWnd = CreateWindowExW(
        0,  // Extended window style
        wc.lpszClassName,
        L"DummyWindow",
        WS_OVERLAPPEDWINDOW,
        0,        // Horizontal position
        0,        // Vertical position
        100,      // Width
        100,      // Height
        nullptr,  // Parent window
        nullptr,  // Menu handle
        wc.hInstance,
        nullptr  // Additional application data
    );
    if (!hDummyWnd) {
        DEBUG_PRINT("[!] Failed to create dummy window.\n");
        return nullptr;
    }

    // Temporary swap chain description
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = 100;
    scd.BufferDesc.Height = 100;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hDummyWnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* pTempDevice = nullptr;
    ID3D11DeviceContext* pTempContext = nullptr;
    IDXGISwapChain* pTempSwapChain = nullptr;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

    // Create a dummy device and swap chain
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,  // Adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,  // Software
        0,        // Flags
        nullptr,  // pFeatureLevels
        0,        // FeatureLevels count
        D3D11_SDK_VERSION,
        &scd,
        &pTempSwapChain,
        &pTempDevice,
        &featureLevel,
        &pTempContext
    );
    if (FAILED(hr)) {
        DestroyWindow(hDummyWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        DEBUG_PRINT("[!] Failed to create dummy device/swap chain. HRESULT = 0x%08X\n", hr);
        return nullptr;
    }

    // Retrieve the Present pointer from the swap chain's vtable
    void** vTable = *reinterpret_cast<void***>(pTempSwapChain);

    // Present is typically at vtable index 8
    void* pPresentAddr = vTable[8];
    DEBUG_PRINT("[+] Dummy Device created. Present @ vtable[8] = 0x%p\n", pPresentAddr);

    // Clean up the dummy objects
    pTempSwapChain->Release();
    pTempDevice->Release();
    pTempContext->Release();

    DestroyWindow(hDummyWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return pPresentAddr;
}

}  // namespace D3D11

}  // namespace Utils
}  // namespace Cynical