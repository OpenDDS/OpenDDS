/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/EventDispatcher.h>

#include <dds/DCPS/ConditionVariable.h>

#include <gtest/gtest.h>

namespace {

class TestEventBase : public OpenDDS::DCPS::EventBase {
public:
  TestEventBase() : cv_(mutex_), call_count_(0), error_count_(0), cancel_count_(0) {}

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

  size_t increment_error_count()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++error_count_;
    cv_.notify_all();
    return error_count_;
  }

  size_t error_count()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return error_count_;
  }

  size_t increment_cancel_count()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++cancel_count_;
    cv_.notify_all();
    return cancel_count_;
  }

  size_t cancel_count()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return cancel_count_;
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
  size_t error_count_;
  size_t cancel_count_;
};

struct SimpleTestEvent : public TestEventBase {
  SimpleTestEvent() {}
  void handle_event() { increment_call_count(); }
  void handle_error() { increment_error_count(); }
  void handle_cancel() { increment_cancel_count(); }
};

struct BrokenTestEvent : public TestEventBase {
  BrokenTestEvent() {}
  void handle_event() { throw 1; }
  void handle_error() { increment_error_count(); }
  void handle_cancel() { increment_cancel_count(); }
};

struct TestEventDispatcher : public OpenDDS::DCPS::EventDispatcher {
  void shutdown(bool) {}
  bool dispatch(OpenDDS::DCPS::EventBase_rch) { return false; }
  long schedule(OpenDDS::DCPS::EventBase_rch, const OpenDDS::DCPS::MonotonicTimePoint&) { return -1; }
  size_t cancel(long) { return 0; }
};

} // (anonymous) namespace

TEST(dds_DCPS_EventDispatcher, EventBaseConstructDestruct)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
}

TEST(dds_DCPS_EventDispatcher, EventBasePassThrough)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();

  test_event->_add_ref();
  (*test_event)();

  test_event->handle_error();
  test_event->handle_error();
  test_event->handle_cancel();
  test_event->handle_cancel();
  test_event->handle_cancel();

  EXPECT_EQ(test_event->call_count(), 1u);
  EXPECT_EQ(test_event->error_count(), 2u);
  EXPECT_EQ(test_event->cancel_count(), 3u);
}

TEST(dds_DCPS_EventDispatcher, EventBaseHandleException)
{
  OpenDDS::DCPS::RcHandle<BrokenTestEvent> test_event = OpenDDS::DCPS::make_rch<BrokenTestEvent>();

  test_event->_add_ref();
  (*test_event)();

  test_event->handle_error();
  test_event->handle_error();
  test_event->handle_cancel();
  test_event->handle_cancel();
  test_event->handle_cancel();

  EXPECT_EQ(test_event->call_count(), 0u);
  EXPECT_EQ(test_event->error_count(), 3u);
  EXPECT_EQ(test_event->cancel_count(), 3u);
}

TEST(dds_DCPS_EventDispatcher, TestEventDispatcher)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<TestEventDispatcher>();
  EXPECT_EQ(dispatcher->dispatch(test_event), false);
  EXPECT_EQ(dispatcher->schedule(test_event, OpenDDS::DCPS::MonotonicTimePoint::now()), -1);
  EXPECT_EQ(dispatcher->cancel(-1), 0u);
  dispatcher->shutdown();
}
