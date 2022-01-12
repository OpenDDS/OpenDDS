#ifndef RTPSRELAY_RELAY_THREAD_MONITOR_H_
#define RTPSRELAY_RELAY_THREAD_MONITOR_H_

#include "Config.h"

#include <dds/OpenddsDcpsExtTypeSupportImpl.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Task.h>

namespace RtpsRelay {

class RelayThreadMonitor : public virtual ACE_Task_Base {
public:
  RelayThreadMonitor(const Config& config,
                     OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var thread_status_reader)
    : config_(config)
    , running_(false)
    , condition_(mutex_)
    , thread_status_reader_(thread_status_reader)
  {}

  int start();
  void stop();

private:
  int svc();

  const Config& config_;
  bool running_;
  mutable ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> condition_;
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var thread_status_reader_;
};

}

#endif // RTPSRELAY_RELAY_THREAD_MONITOR_H_
