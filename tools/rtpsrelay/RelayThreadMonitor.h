#ifndef RTPSRELAY_RELAY_THREAD_MONITOR_H_
#define RTPSRELAY_RELAY_THREAD_MONITOR_H_

#include "Config.h"
#include "ListenerBase.h"

#include <dds/OpenddsDcpsExtTypeSupportImpl.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Task.h>

namespace RtpsRelay {

class RelayThreadMonitor : public virtual ACE_Task_Base, public ListenerBase {
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

  bool threads_okay() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    for (const auto& u : utilization_) {
      if (u.second > config_.utilization_limit()) {
        return false;
      }
    }

    return true;
  }

private:
  int svc() override;
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  const Config& config_;
  bool running_;
  std::map<std::string, double> utilization_;
  mutable ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> condition_;
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var thread_status_reader_;
};

}

#endif // RTPSRELAY_RELAY_THREAD_MONITOR_H_
