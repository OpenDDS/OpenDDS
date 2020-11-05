#include "ProcessStatsCollector.h"

#ifdef ACE_WIN32
#include <windows.h>
#include <psapi.h>
#endif

ProcessStatsCollector::ProcessStatsCollector(const int process_id) noexcept
  : process_id_(process_id)
  , num_processors_(0)
  , total_virtual_mem_(0)
  , total_mem_(0)
#ifdef ACE_WIN32
  , process_handle_(INVALID_HANDLE_VALUE)
  , last_time_()
  , last_sys_time_()
  , last_user_time_()
#endif
{
#ifdef ACE_WIN32
  SYSTEM_INFO sysInfo{};
  FILETIME ftime{}, fcreatetime{}, fexittime{}, fsys{}, fuser{};

  GetSystemInfo(&sysInfo);

  num_processors_ = sysInfo.dwNumberOfProcessors;

  GetSystemTimeAsFileTime(&ftime);

  memcpy(&last_time_, &ftime, sizeof(FILETIME));

  process_handle_ = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)process_id_);

  GetProcessTimes(process_handle_, &fcreatetime, &fexittime, &fsys, &fuser);

  memcpy(&last_sys_time_, &fsys, sizeof(FILETIME));
  memcpy(&last_user_time_, &fuser, sizeof(FILETIME));

  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  if (GlobalMemoryStatusEx(&memInfo)) {
    total_virtual_mem_ = memInfo.ullTotalPageFile;
    total_mem_ = memInfo.ullTotalPhys;
  }
#endif
}

ProcessStatsCollector::~ProcessStatsCollector() noexcept
{
#ifdef ACE_WIN32
  CloseHandle(process_handle_);
#endif
}

double ProcessStatsCollector::get_cpu_usage() noexcept
{
  double percent = 0.0;
#ifdef ACE_WIN32
  FILETIME ftime{}, fsys{}, fuser{};
  ULARGE_INTEGER current{}, sys{}, user{};

  GetSystemTimeAsFileTime(&ftime);

  memcpy(&current, &ftime, sizeof(FILETIME));

  if (GetProcessTimes(process_handle_, &ftime, &ftime, &fsys, &fuser)) {
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    percent = 100.0 * static_cast<double>((sys.QuadPart - last_sys_time_.QuadPart) + (user.QuadPart - last_user_time_.QuadPart));
    percent /= num_processors_ * (current.QuadPart - last_time_.QuadPart);

    last_time_ = current;
    last_sys_time_ = sys;
    last_user_time_ = user;
  }
#endif
  return percent;
}

double ProcessStatsCollector::get_virtual_mem_usage() noexcept
{
  double result = 0;
#ifdef ACE_WIN32
  try {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(process_handle_, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
      const size_t used = pmc.PrivateUsage;
      if (total_virtual_mem_ > 0) {
        result = 100.0 * (static_cast<double>(used) / static_cast<double>(total_virtual_mem_));
      }
    }
  } catch (...) {}
#endif
  return result;
}

double ProcessStatsCollector::get_mem_usage() noexcept
{
  double result = 0.0;
#ifdef ACE_WIN32
  try {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(process_handle_, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
      const size_t used = pmc.PeakWorkingSetSize;
      if (total_mem_ > 0) {
        result = 100.0 * (static_cast<double>(used) / static_cast<double>(total_mem_));
      }
    }
  } catch (...) {}
#endif
  return result;
}
