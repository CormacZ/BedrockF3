// src/f3_debug/system_info/SystemInfo.cpp

#include "f3_debug/system_info/SystemInfo.h"

#include <Windows.h>
#include <dxgi1_4.h>
#include <psapi.h>

#include <cstring>
#include <string>

namespace f3_debug::system {

namespace {

// Convert a wide C string to UTF-8. Returns an empty string on
// failure (e.g. null input or invalid characters).
std::string wideToUtf8(wchar_t const* wstr) {
    if (wstr == nullptr || wstr[0] == L'\0') {
        return {};
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }
    std::string out(static_cast<std::size_t>(needed - 1), '\0');
    int written = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out.data(), needed, nullptr, nullptr);
    if (written <= 0) {
        return {};
    }
    return out;
}

// Convert bytes to a human-readable MB count, rounded down.
// e.g. 902283468 -> 860. Computes the divisor in int64_t
// explicitly so the bugprone-implicit-widening-of-multiplication
// clang-tidy check does not fire (the literal 1024 is int by
// default, so 1024 * 1024 is int, which would silently truncate
// on values above 2 GiB before the division even happens).
constexpr std::int64_t bytesToMB(std::int64_t bytes) noexcept {
    return bytes / (std::int64_t{1024} * 1024);
}

} // namespace

std::string cpuName() {
    // The CPU brand string is stored in the registry under the
    // CentralProcessor\0 key. This is the same string Windows
    // shows in System Information. The key exists on every
    // Windows install from XP onwards.
    HKEY hKey = nullptr;
    LSTATUS openRc =
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey);
    if (openRc != ERROR_SUCCESS) {
        return "Unknown CPU";
    }

    wchar_t name[256] = {};
    DWORD size = sizeof(name);
    LSTATUS queryRc =
        RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr, reinterpret_cast<LPBYTE>(name), &size);
    RegCloseKey(hKey);

    if (queryRc != ERROR_SUCCESS) {
        return "Unknown CPU";
    }
    return wideToUtf8(name);
}

std::string gpuName() {
    // Use the DXGI factory to enumerate graphics adapters and
    // pull the friendly name from the first one (index 0). The
    // first adapter is the primary display adapter on most
    // systems. DXGI does not require an explicit CoInitializeEx
    // call -- CreateDXGIFactory1 initializes COM internally.
    IDXGIFactory4* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr) || factory == nullptr) {
        return "Unknown GPU";
    }

    std::string result = "Unknown GPU";
    IDXGIAdapter1* adapter = nullptr;
    HRESULT enumRc = factory->EnumAdapters1(0, &adapter);
    if (SUCCEEDED(enumRc) && adapter != nullptr) {
        DXGI_ADAPTER_DESC1 desc = {};
        HRESULT descRc = adapter->GetDesc1(&desc);
        if (SUCCEEDED(descRc)) {
            // desc.Description is a wchar_t[128] (null-terminated).
            // It can have trailing whitespace that we want to
            // trim. We don't have a built-in trim, so do it
            // manually with std::string.
            std::string name = wideToUtf8(desc.Description);
            auto end = name.find_last_not_of(" \t\r\n");
            if (end != std::string::npos) {
                name.erase(end + 1);
            } else {
                name.clear();
            }
            if (!name.empty()) {
                result = name;
            }
        }
        adapter->Release();
    }
    factory->Release();
    return result;
}

std::int64_t processMemoryUsed() {
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<std::int64_t>(pmc.WorkingSetSize);
    }
    return 0;
}

std::int64_t totalMemory() {
    MEMORYSTATUSEX memInfo = {};
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return static_cast<std::int64_t>(memInfo.ullTotalPhys);
    }
    return 0;
}

std::int64_t availableMemory() {
    MEMORYSTATUSEX memInfo = {};
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return static_cast<std::int64_t>(memInfo.ullAvailPhys);
    }
    return 0;
}

} // namespace f3_debug::system
