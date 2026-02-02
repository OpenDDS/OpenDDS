#ifndef OPENDDS_DCPS_RTPS_THREADSTATUSHARVESTER_H
#define OPENDDS_DCPS_RTPS_THREADSTATUSHARVESTER_H

#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/RTPS/Spdp.h>

#include <ace/Task.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

class ThreadStatusHarvester : public virtual ACE_Task_Base {
public:
  explicit ThreadStatusHarvester(DCPS::RcHandle<Spdp> spdp);
  ~ThreadStatusHarvester();

  int start();
  void stop();

private:
  int svc() override;

  bool running_;

  // While the thread is running (running_ == true), harvesting can be disabled.
  bool disable_;
  ACE_Thread_Mutex mutex_;
  DCPS::ConditionVariable<ACE_Thread_Mutex> condition_;
  DCPS::WeakRcHandle<Spdp> spdp_;
  DCPS::MonotonicTimePoint last_harvest_;
};
typedef DCPS::RcHandle<ThreadStatusHarvester> ThreadStatusHarvester_rch;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
