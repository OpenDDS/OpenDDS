#include "ProcessStatsCollector.h"

#ifdef ACE_WIN32
#include <windows.h>
#include <psapi.h>
#elif defined ACE_LINUX
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

#if defined ACE_LINUX
namespace {

bool read_total_cpu_usage(size_t& time)
{
  std::string line;
  std::string filename = "/proc/stat";

  std::ifstream statfile;
  statfile.open(filename, std::ios::in);
  if (!statfile.is_open()) {
    return false;
  }

  size_t user_time = 0;
  size_t nice_time = 0;
  size_t system_time = 0;
  size_t idle_time = 0;

  std::stringstream ss;
  std::string cpuname;

  getline(statfile, line);

  ss << line;
  ss >> cpuname;

  if (cpuname != "cpu") {
    return false;
  }
  ss >> user_time;
  ss >> nice_time;
  ss >> system_time;
  ss >> idle_time;

  statfile.close();

  const size_t new_time = user_time + nice_time + system_time + idle_time;

  if (!new_time) {
    return false;
  }

  time = new_time;
  return true;
}

bool read_process_cpu_usage(int processId, size_t& utime, size_t& stime)
{
  std::string line;
  std::string filename = "/proc/" + std::to_string(processId) + "/stat";

  std::ifstream statfile;
  statfile.open(filename, std::ios::in);
  if (!statfile.is_open()) {
    return false;
  }

  int pid;
  std::string comm;
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  size_t flags;
  size_t minflt;
  size_t cminflt;
  size_t majflt;
  size_t cmajflt;

  std::stringstream ss;
  std::getline(statfile, line);

  ss << line;
  ss >> pid;
  ss >> comm;
  ss >> state;
  ss >> ppid;
  ss >> pgrp;
  ss >> session;
  ss >> tty_nr;
  ss >> tpgid;
  ss >> flags;
  ss >> minflt;
  ss >> cminflt;
  ss >> majflt;
  ss >> cmajflt;
  ss >> utime;
  ss >> stime;

  statfile.close();

  return true;
}

} // namespace {
#endif // ACE_LINUX

ProcessStatsCollector::ProcessStatsCollector(const int process_id) noexcept
#ifndef ACE_HAS_MAC_OSX
  : process_id_(process_id)
  , num_processors_(0)
  , total_virtual_mem_(0)
  , total_mem_(0)
#endif
#ifdef ACE_WIN32
  , process_handle_(INVALID_HANDLE_VALUE)
  , last_time_()
  , last_sys_time_()
  , last_user_time_()
#elif defined ACE_LINUX
  , last_time_(0)
  , last_sys_time_(0)
  , last_user_time_(0)
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
#elif defined ACE_LINUX
  std::ifstream meminfofile;
  meminfofile.open("/proc/meminfo", ios::in);

  if (meminfofile.is_open()) {
    std::string token;
    while (meminfofile >> token) {
      if (token == "MemTotal:") {
        meminfofile >> total_mem_;
      }
      if (token == "VmallocTotal:") {
        meminfofile >> total_virtual_mem_;
      }
      if (total_mem_ != 0 && total_virtual_mem_ != 0) {
        break;
      }
    }
    meminfofile.close();
  }
  read_total_cpu_usage(last_time_);
  read_process_cpu_usage(process_id_, last_user_time_, last_sys_time_);
#else
  ACE_UNUSED_ARG(process_id);
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
#elif defined ACE_LINUX
  size_t time{}, sys_time{}, user_time{};
  if (read_total_cpu_usage(time) &&
      time > last_time_ &&
      read_process_cpu_usage(process_id_, user_time, sys_time)) {

    if (time > last_time_) {
      percent = 100.0 * static_cast<double>((sys_time - last_sys_time_) + (user_time - last_user_time_));
      percent /= time - last_time_; // total cpu time is across all processors (ignore num_processors_)

      last_time_ = time;
      last_sys_time_ = sys_time;
      last_user_time_ = user_time;
    }
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
#elif defined ACE_LINUX
  std::string filename = "/proc/" + std::to_string(process_id_) + "/statm";

  std::ifstream statmfile;
  statmfile.open(filename, ios::in);

  long page_sz_kb = sysconf(_SC_PAGESIZE) / 1024;

  if (statmfile.is_open()) {
    long VmSize{};
    statmfile >> VmSize;

    VmSize *= page_sz_kb;

    if (total_virtual_mem_ > 0) {
      result = 100.0 * (static_cast<double>(VmSize) / static_cast<double>(total_virtual_mem_));
    }
    statmfile.close();
  }
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
#elif defined ACE_LINUX
  std::string filename = "/proc/" + std::to_string(process_id_) + "/statm";

  std::ifstream statmfile;
  statmfile.open(filename, ios::in);

  long page_sz_kb = sysconf(_SC_PAGESIZE) / 1024;

  if (statmfile.is_open()) {
    long VmSize{}, VmRSS{};
    statmfile >> VmSize >> VmRSS;

    VmRSS *= page_sz_kb;

    if (total_mem_ > 0) {
      result = 100.0 * (static_cast<double>(VmRSS) / static_cast<double>(total_mem_));
    }
    statmfile.close();
  }
#endif
  return result;
}
