#pragma once

#include <cstdio>
#include <initializer_list>

#include <Windows.h>

namespace Cynical {
namespace Utils {

#ifdef _DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

namespace Memory {

template <typename T>
void Write(__int64 baseAddress, T value, std::initializer_list<__int64> offsets = {});

template <typename K>
K Read(__int64 baseAddress, std::initializer_list<__int64> offsets = {});

}  // namespace Memory

namespace D3D11 {
void* GetPresentAddress();
}  // namespace D3D11

}  // namespace Utils
}  // namespace Cynical