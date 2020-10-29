#pragma once

#include <windows.h>
#include <psapi.h>
#include <stdio.h>

DWORDLONG GetTotalVirtualMemory();
DWORDLONG GetVirtualMemoryUsed();
DWORDLONG GetProcessVirtualMemoryUsed(int ProcessId);
DWORDLONG GetTotalRamMemory();
DWORDLONG GetTotalRamMemoryUsed();
DWORDLONG GetTotalRamMemoryUsedByProcess(int ProcessId);
