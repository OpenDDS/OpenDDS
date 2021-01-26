#pragma once

#include <BenchTypeSupportImpl.h>

class ProcessStatsCollector {
public:
  ProcessStatsCollector(const int processId) noexcept;

  ProcessStatsCollector() = delete;
  ProcessStatsCollector(const ProcessStatsCollector&) = delete;
  ProcessStatsCollector(ProcessStatsCollector&&) = delete;
  ProcessStatsCollector& operator=(const ProcessStatsCollector&) = delete;
  ProcessStatsCollector& operator=(ProcessStatsCollector&&) = delete;

  ~ProcessStatsCollector() noexcept;

  double get_cpu_usage() noexcept;

  double get_virtual_mem_usage() noexcept;

  double get_mem_usage() noexcept;

private:
#ifndef ACE_HAS_MAC_OSX
  int process_id_;
  size_t num_processors_;
  uint64_t total_virtual_mem_;
  uint64_t total_mem_;
#endif
#ifdef ACE_WIN32
  HANDLE process_handle_;
  ULARGE_INTEGER last_time_;
  ULARGE_INTEGER last_sys_time_;
  ULARGE_INTEGER last_user_time_;
#elif defined ACE_LINUX
  size_t last_time_;
  size_t last_sys_time_;
  size_t last_user_time_;
#endif
};
