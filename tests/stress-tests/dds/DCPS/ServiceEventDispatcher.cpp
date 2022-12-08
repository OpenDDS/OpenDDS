/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/ServiceEventDispatcher.h>

#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/ThreadStatusManager.h>

#include <gtest/gtest.h>

namespace {

class TestEventBase : public OpenDDS::DCPS::EventBase {
public:
  TestEventBase() : cv_(mutex_), call_count_(0) {}

  size_t increment_call_count()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++call_count_;
    cv_.notify_all();
    return call_count_;
  }

  size_t call_count()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return call_count_;
  }

  void wait(size_t target)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    while (call_count_ < target) {
      cv_.wait(tsm_);
    }
  }

private:
  ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> cv_;
  OpenDDS::DCPS::ThreadStatusManager tsm_;
  size_t call_count_;
};

struct SimpleTestEvent : public TestEventBase {
  SimpleTestEvent() {}
  void handle_event() { increment_call_count(); }
};

struct RecursiveTestEventOne : public TestEventBase {
  RecursiveTestEventOne(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher) : dispatcher_(dispatcher) {}

  void handle_event()
  {
    if (increment_call_count() % 2) {
      OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = dispatcher_.lock();
      if (dispatcher) {
        dispatcher->dispatch(OpenDDS::DCPS::rchandle_from(this));
      }
    }
  }

  OpenDDS::DCPS::WeakRcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher_;
};

struct RecursiveTestEventTwo : public TestEventBase {
  RecursiveTestEventTwo(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher, size_t dispatch_scale) : dispatcher_(dispatcher), dispatch_scale_(dispatch_scale) {}

  void handle_event()
  {
    increment_call_count();
    const size_t scale = dispatch_scale_.value();
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = dispatcher_.lock();
    if (dispatcher) {
      for (size_t i = 0; i < scale; ++i) {
        dispatcher->dispatch(OpenDDS::DCPS::rchandle_from(this));
      }
    }
  }

  OpenDDS::DCPS::WeakRcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> dispatch_scale_;
};

} // (anonymous) namespace

TEST(dds_DCPS_ServiceEventDispatcher, RecursiveDispatchDelta)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>(8);
  OpenDDS::DCPS::RcHandle<RecursiveTestEventTwo> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventTwo>(dispatcher, 2);

  dispatcher->dispatch(test_event);

  test_event->wait(100000u);
  test_event->dispatch_scale_ = 0;
  dispatcher->shutdown();

  EXPECT_GE(test_event->call_count(), 100000u);
}

TEST(dds_DCPS_ServiceEventDispatcher, RecursiveDispatchDelta_ImmediateShutdown)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>(8);
  OpenDDS::DCPS::RcHandle<RecursiveTestEventTwo> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventTwo>(dispatcher, 2);

  dispatcher->dispatch(test_event);

  test_event->wait(100000u);
  test_event->dispatch_scale_ = 0;
  dispatcher->shutdown(true);

  EXPECT_GE(test_event->call_count(), 100000u);
}
