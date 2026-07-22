// src/f3_debug/system_info/SystemInfo.h
// Cross-platform-shaped interface for system info.
//
// We currently only target Windows (LeviLamina + LeviLauncher are
// Windows-only, and Bedrock itself is Windows/Xbox/console). The
// API here is designed to be portable -- the right column of the
// F3 panel would work fine on Linux/macOS too if LeviLamina ever
// expands there -- but for now every function is implemented by
// hitting the Windows API directly. If/when we need portability,
// wrap each implementation in #ifdef _WIN32 and add a Linux/macOS
// path (or switch to a library like hwinfo).
//
// The current Windows implementations are:
//
//   * CPU brand     -> HKLM\HARDWARE\DESCRIPTION\System\...
//                       CentralProcessor\0\ProcessorNameString
//   * GPU name      -> IDXGIAdapter::GetDesc1() on adapter index 0
//   * Process memory-> GetProcessMemoryInfo(GetCurrentProcess(), ...)
//   * Total memory  -> GlobalMemoryStatusEx().ullTotalPhys
//   * Avail memory  -> GlobalMemoryStatusEx().ullAvailPhys

#pragma once

#include <cstdint>
#include <string>

namespace f3_debug::system {

// CPU brand string. On Windows, read from the registry:
// "12th Gen Intel(R) Core(TM) i5-12600K", "AMD Ryzen 7 5800X", etc.
// Returns "Unknown CPU" if the lookup fails.
[[nodiscard]] std::string cpuName();

// GPU adapter name (the friendly name, not the driver name). On
// Windows, returned by IDXGIAdapter1::GetDesc1 on the first
// adapter: "NVIDIA GeForce RTX 3060", "AMD Radeon RX 6700 XT",
// "Intel(R) UHD Graphics 770", etc.
// Returns "Unknown GPU" if the lookup fails.
[[nodiscard]] std::string gpuName();

// Process working set in bytes (physical memory currently
// committed to this process). On Windows, returned by
// GetProcessMemoryInfo's WorkingSetSize.
[[nodiscard]] std::int64_t processMemoryUsed();

// Total physical RAM in bytes. On Windows, returned by
// GlobalMemoryStatusEx's ullTotalPhys.
[[nodiscard]] std::int64_t totalMemory();

// Physical memory currently free (available to the OS). On
// Windows, returned by GlobalMemoryStatusEx's ullAvailPhys.
[[nodiscard]] std::int64_t availableMemory();

} // namespace f3_debug::system
