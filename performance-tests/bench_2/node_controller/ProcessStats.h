#pragma once

#include <BenchTypeSupportImpl.h>

class ProcessStats {
public:
  ProcessStats(const int& processId, const Bench::NodeController::NodeId& nodeId, const Bench::NodeController::WorkerId& workerId) noexcept;

  ProcessStats() = delete;
  ProcessStats(const ProcessStats&) = delete;
  ProcessStats(ProcessStats&&) = delete;
  ProcessStats& operator=(const ProcessStats&) = delete;
  ProcessStats& operator=(ProcessStats&&) = delete;

  ~ProcessStats() noexcept;

  double GetProcessCPUUsage() noexcept;

  double GetProcessPercentVirtualUsed();

  double GetProcessPercentRamUsed();

private:
  int processId_;
  int numProcessors_;
  Bench::NodeController::NodeId node_id_;
  Bench::NodeController::WorkerId worker_id_;
#ifdef ACE_WIN32
  HANDLE processHandle_;
  ULARGE_INTEGER lastCPU_;
  ULARGE_INTEGER lastSysCPU_;
  ULARGE_INTEGER lastUserCPU_;
#endif
};

