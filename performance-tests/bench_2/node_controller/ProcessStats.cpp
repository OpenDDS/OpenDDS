#include "ProcessStats.h"

#include "ProcessStatisticsUtils.h"

ProcessStats::ProcessStats(const int& processId, const Bench::NodeController::NodeId& nodeId, const Bench::NodeController::WorkerId& workerId) noexcept
  : processId_(processId)
  , numProcessors_(0)
  , node_id_(nodeId)
  , worker_id_(workerId)
#ifdef ACE_WIN32
  , processHandle_(INVALID_HANDLE_VALUE)
  , lastCPU_()
  , lastSysCPU_()
  , lastUserCPU_()
#endif
{
#ifdef ACE_WIN32
  SYSTEM_INFO sysInfo{};
  FILETIME ftime{}, fcreatetime{}, fexittime{}, fsys{}, fuser{};

  GetSystemInfo(&sysInfo);

  numProcessors_ = sysInfo.dwNumberOfProcessors;

  GetSystemTimeAsFileTime(&ftime);

  memcpy(&lastCPU_, &ftime, sizeof(FILETIME));

  processHandle_ = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)processId_);

  GetProcessTimes(processHandle_, &fcreatetime, &fexittime, &fsys, &fuser);
  memcpy(&lastSysCPU_, &fsys, sizeof(FILETIME));
  memcpy(&lastUserCPU_, &fuser, sizeof(FILETIME));
#endif
}

ProcessStats::~ProcessStats() noexcept {
#ifdef ACE_WIN32
  CloseHandle(processHandle_);
#endif
}

double ProcessStats::GetProcessCPUUsage() noexcept
{
  double percent = 0.0;
#ifdef ACE_WIN32
  FILETIME ftime{}, fsys{}, fuser{};
  ULARGE_INTEGER current{}, sys{}, user{};

  GetSystemTimeAsFileTime(&ftime);
  memcpy(&current, &ftime, sizeof(FILETIME));

  if (GetProcessTimes(processHandle_, &ftime, &ftime, &fsys, &fuser))
  {
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    percent = static_cast<double>((sys.QuadPart - lastSysCPU_.QuadPart) + (user.QuadPart - lastUserCPU_.QuadPart));
    percent /= static_cast<double>((current.QuadPart - lastCPU_.QuadPart));
    percent /= static_cast<double>(numProcessors_);

    lastCPU_ = current;
    lastUserCPU_ = user;
    lastSysCPU_ = sys;
  }
#endif
  return percent * 100.0;
}

double ProcessStats::GetProcessPercentVirtualUsed() {
  double percentProcessVirtualUsed = 0;
#ifdef ACE_WIN32
  DWORDLONG const totalVirtual = GetTotalVirtualMemory();
  DWORDLONG const processVirtualUsed = GetProcessVirtualMemoryUsed(processId_);

  if (totalVirtual > 0) {
    percentProcessVirtualUsed = (static_cast<double>(processVirtualUsed) / static_cast<double>(totalVirtual)) * 100.00;
  }
#endif
  return percentProcessVirtualUsed;
}

double ProcessStats::GetProcessPercentRamUsed()
{
  double percentProcessRamUsed = 0;
#ifdef ACE_WIN32
  DWORDLONG const totalRam = GetTotalRamMemory();
  DWORDLONG const processRamUsed = GetTotalRamMemoryUsedByProcess(processId_);

  if (totalRam > 0) {
    percentProcessRamUsed = (static_cast<double>(processRamUsed) / static_cast<double>(totalRam)) * 100.00;
  }
#endif
  return percentProcessRamUsed;
}

