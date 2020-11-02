#include "ProcessStatisticsUtils.h"

#ifdef ACE_WIN32
#include <windows.h>
#include <psapi.h>
#endif

int64_t GetTotalVirtualMemory()
{
#ifdef ACE_WIN32
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPageFile;
#else
  return 0;
#endif
}

int64_t GetVirtualMemoryUsed()
{
#ifdef ACE_WIN32
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
#else
  return 0;
#endif
}

int64_t GetProcessVirtualMemoryUsed(int ProcessId)
{
#ifdef ACE_WIN32
  PROCESS_MEMORY_COUNTERS_EX pmc;
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
  SIZE_T virtualMemUsedByProcess = pmc.PrivateUsage;
  return virtualMemUsedByProcess;
#else
  return 0 * ProcessId;
#endif
}

int64_t GetTotalRamMemory()
{
#ifdef ACE_WIN32
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPhys;
#else
  return 0;
#endif
}

int64_t GetTotalRamMemoryUsed()
{
#ifdef ACE_WIN32
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  return memInfo.ullTotalPhys - memInfo.ullAvailPhys;
#else
  return 0;
#endif
}

int64_t GetTotalRamMemoryUsedByProcess(int ProcessId)
{
#ifdef ACE_WIN32
  PROCESS_MEMORY_COUNTERS_EX pmc;

  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
  SIZE_T RamMemUsedByProcess = pmc.PeakWorkingSetSize;
  return RamMemUsedByProcess;
#else
  return 0 * ProcessId;
#endif
}
