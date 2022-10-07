#include <dds/DCPS/ServiceEventDispatcher.h>
#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/ThreadStatusManager.h>
#include <dds/DCPS/AtomicBool.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

namespace {

class InvertEvent : public EventBase {
public:
  AtomicBool value_;
  ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> cv_;
  OpenDDS::DCPS::ThreadStatusManager tsm_;
  unsigned call_count_;

  InvertEvent()
  : value_(false)
  , cv_(mutex_)
  , call_count_(0)
  {
  }

  void handle_event()
  {
    value_ = !value_;

    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++call_count_;
    cv_.notify_all();
  }

  void wait(unsigned target)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    while (call_count_ < target) {
      cv_.wait(tsm_);
    }
  }
};

}

TEST(dds_DCPS_AtomicBool, tsan_test)
{
  RcHandle<InvertEvent> event = make_rch<InvertEvent>();
  RcHandle<EventDispatcher> dispatcher = make_rch<ServiceEventDispatcher>(8);

  const unsigned count = 32;
  for (unsigned i = 0; i < count; ++i) {
    dispatcher->dispatch(event);
  }
  event->wait(count);

  dispatcher->shutdown();
}
