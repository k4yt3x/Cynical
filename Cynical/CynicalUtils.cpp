#include "CynicalUtils.h"

#include <Windows.h>
#include <d3d11.h>

namespace Cynical {
namespace Utils {

namespace Memory {

__int64 ResolveAddress(__int64 baseAddress, std::initializer_list<__int64> offsets) {
    if (!baseAddress) {
        return 0;
    }

    // Dereference the base address to get the first address
    __int64 address = *reinterpret_cast<__int64*>(baseAddress);

    size_t currentIndex = 0;
    for (auto offset : offsets) {
        if (!address) {
            return 0;
        }

        if (currentIndex < offsets.size() - 1) {
            address = *reinterpret_cast<__int64*>(address + offset);
        } else {
            address += offset;
        }
        currentIndex++;
    }

    return address;
}

template <typename T>
void Write(__int64 address, T value, bool changeProt) {
    DWORD oldProtection;
    if (changeProt){
        VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtection);
    }
    
    *reinterpret_cast<T*>(address) = value;

    if (changeProt) {
        VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtection, &oldProtection);
    }
}

template <typename K>
K Read(__int64 address) {
    return *reinterpret_cast<K*>(address);
}

void WriteBytes(__int64 address, const std::vector<BYTE>& bytes, bool changeProt) {
    DWORD oldProtection;
    if (changeProt) {
        VirtualProtect(reinterpret_cast<void*>(address), bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtection);
    }

    memcpy(reinterpret_cast<void*>(address), bytes.data(), bytes.size());
    
    if (changeProt) {
        VirtualProtect(reinterpret_cast<void*>(address), bytes.size(), oldProtection, &oldProtection);
    }
}

std::vector<BYTE> ReadBytes(__int64 address, SIZE_T numBytes) {
    std::vector<BYTE> bytes(numBytes);
    memcpy(bytes.data(), reinterpret_cast<void*>(address), numBytes);
    return bytes;
}

}  // namespace Memory

}  // namespace Utils
}  // namespace Cynical