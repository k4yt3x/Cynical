#pragma once

#include <cstdio>
#include <initializer_list>
#include <vector>

#include <Windows.h>

namespace Cynical {
namespace Utils {

#ifdef _DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

namespace Memory {

__int64 ResolveAddress(__int64 baseAddress, std::initializer_list<__int64> offsets);

template <typename T>
void Write(__int64 address, T value);

template <typename K>
K Read(__int64 address);

void WriteBytes(__int64 address, const std::vector<BYTE>& bytes);

std::vector<BYTE> ReadBytes(__int64 address, SIZE_T numBytes);

}  // namespace Memory

namespace D3D11 {
void* GetPresentAddress();
}  // namespace D3D11

}  // namespace Utils
}  // namespace Cynical