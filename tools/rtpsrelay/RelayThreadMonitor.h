#ifndef RTPSRELAY_RELAY_THREAD_MONITOR_H_
#define RTPSRELAY_RELAY_THREAD_MONITOR_H_

#include "Config.h"
#include "ReaderListenerBase.h"

#include <dds/OpenddsDcpsExtTypeSupportImpl.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Task.h>

namespace RtpsRelay {

class RelayThreadMonitor : public virtual ACE_Task_Base, public ReaderListenerBase {
public:
  explicit RelayThreadMonitor(const Config& config)
    : config_(config)
    , running_(false)
    , condition_(mutex_)
  {}

  void set_reader(OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var thread_status_reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    thread_status_reader_ = thread_status_reader;
  }

  int start();
  void stop();

  bool threads_okay() const;

private:
  int svc() override;
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;
  std::string decompose_thread_detail1(int detail) const;

  const Config& config_;
  bool running_;

  struct UtilizationRecord {
    // latest recorded utilization
    double utilization = 0.0;

    // number of consecutive times the utilization exceeded the limit
    unsigned int exceed_limit_count = 0;

    void record(double util, double limit)
    {
      utilization = util;
      if (util > limit) {
        ++exceed_limit_count;
      } else {
        exceed_limit_count = 0;
      }
    }
  };
  std::map<std::string, UtilizationRecord> utilization_;

  mutable ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> condition_;
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var thread_status_reader_;
};

}

#endif // RTPSRELAY_RELAY_THREAD_MONITOR_H_
