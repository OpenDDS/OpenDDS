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
  int process_id_;
  size_t num_processors_;
  size_t total_virtual_mem_;
  size_t total_mem_;
#ifdef ACE_WIN32
  HANDLE process_handle_;
  ULARGE_INTEGER last_time_;
  ULARGE_INTEGER last_sys_time_;
  ULARGE_INTEGER last_user_time_;
#endif
};

