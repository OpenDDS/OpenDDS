#include "ThreadStatusHarvester.h"

#include "dds/DCPS/Service_Participant.h"

namespace OpenDDS {
namespace RTPS {

using namespace OpenDDS::DCPS;

ThreadStatusHarvester::ThreadStatusHarvester(RcHandle<Spdp> spdp)
  : running_(false)
  , disable_(false)
  , condition_(mutex_)
  , spdp_(spdp)
{
}

ThreadStatusHarvester::~ThreadStatusHarvester()
{
  stop();
}

int ThreadStatusHarvester::start()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, -1);

  if (running_) {
    return 0;
  }
  running_ = true;

  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ThreadStatusHarvester::start Failed to activate\n"));
    return -1;
  }

  return 0;
}

void ThreadStatusHarvester::stop()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    running_ = false;
  }
  condition_.notify_all();

  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  ThreadStatusManager::Sleeper s(thread_status_manager);
  wait();
}

void ThreadStatusHarvester::enable()
{
  if (!disable_) {
    // Updated thread status interval is picked up via ThreadStatusManager
    return;
  }

  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    disable_ = false;
  }
  condition_.notify_all();
}

void ThreadStatusHarvester::disable()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  disable_ = true;
}

int ThreadStatusHarvester::svc()
{
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  ThreadStatusManager::Start s(thread_status_manager, "ThreadStatusHarvester");

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, -1);

  while (running_) {
    condition_.wait_until(MonotonicTimePoint::now() + thread_status_manager.thread_status_interval(), thread_status_manager);
    if (!running_) {
      break;
    }
    while (running_ && disable_) {
      condition_.wait(thread_status_manager);
    }
    if (!running_) {
      break;
    }

    RcHandle<Spdp> spdp = spdp_.lock();
    if (!spdp) {
      break;
    }

    typedef ThreadStatusManager::List List;
    List running, removed;
    TheServiceParticipant->get_thread_status_manager().harvest(last_harvest_, running, removed);
    last_harvest_ = MonotonicTimePoint::now();
    for (List::const_iterator i = removed.begin(); i != removed.end(); ++i) {
      DCPS::InternalThreadBuiltinTopicData data;
      data.thread_id = i->bit_key().c_str();
      spdp->bit_subscriber_->remove_thread_status(data);
    }
    for (List::const_iterator i = running.begin(); i != running.end(); ++i) {
      DCPS::InternalThreadBuiltinTopicData data;
      data.thread_id = i->bit_key().c_str();
      data.utilization = i->utilization(now);
      data.monotonic_timestamp = i->last_update().to_idl_struct();
      data.detail1 = i->detail1();
      data.detail2 = i->detail2();
      spdp->bit_subscriber_->add_thread_status(data, DDS::NEW_VIEW_STATE, i->timestamp());
    }
  }
  return 0;
}

}
}
