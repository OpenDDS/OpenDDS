#include "ProcessStatisticsUtils.h"

DWORDLONG GetTotalVirtualMemory()
{
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPageFile;
}

DWORDLONG GetVirtualMemoryUsed()
{
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
}

DWORDLONG GetProcessVirtualMemoryUsed(int ProcessId)
{
  PROCESS_MEMORY_COUNTERS_EX pmc;
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
  SIZE_T virtualMemUsedByProcess = pmc.PrivateUsage;
  return virtualMemUsedByProcess;
}

DWORDLONG GetTotalRamMemory()
{
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPhys;
}

DWORDLONG GetTotalRamMemoryUsed()
{
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPhys - memInfo.ullAvailPhys;
}

DWORDLONG GetTotalRamMemoryUsedByProcess(int ProcessId)
{
  PROCESS_MEMORY_COUNTERS_EX pmc;

  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
  SIZE_T RamMemUsedByProcess = pmc.PeakWorkingSetSize;
  return RamMemUsedByProcess;
}
