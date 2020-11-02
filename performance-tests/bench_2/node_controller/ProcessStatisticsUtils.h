#pragma once

#include <cstdint>

int64_t GetTotalVirtualMemory();
int64_t GetVirtualMemoryUsed();
int64_t GetProcessVirtualMemoryUsed(int ProcessId);
int64_t GetTotalRamMemory();
int64_t GetTotalRamMemoryUsed();
int64_t GetTotalRamMemoryUsedByProcess(int ProcessId);
