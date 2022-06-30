/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/PeriodicEvent.h>

#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/ServiceEventDispatcher.h>

#include <gtest/gtest.h>

namespace
{

class TestEventBase : public OpenDDS::DCPS::EventBase
{
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

struct SimpleTestEvent : public TestEventBase
{
  SimpleTestEvent() {}
  void handle_event() { increment_call_count(); }
};

} // (anonymous) namespace

TEST(dds_DCPS_PeriodicEvent, ConstructDestruct)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::PeriodicEvent> periodic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::PeriodicEvent>(dispatcher, test_event);
}

TEST(dds_DCPS_PeriodicEvent, Nominal)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::PeriodicEvent> periodic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::PeriodicEvent>(dispatcher, test_event);

  EXPECT_EQ(periodic->enabled(), false);

  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000));

  EXPECT_EQ(periodic->enabled(), true);

  // First call should happen immediately due to default args

  test_event->wait(2);
  EXPECT_EQ(test_event->call_count(), 2u);

  test_event->wait(3);
  EXPECT_EQ(test_event->call_count(), 3u);

  periodic->disable();

  EXPECT_EQ(periodic->enabled(), false);

  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, true);

  EXPECT_EQ(periodic->enabled(), true);

  test_event->wait(4);
  EXPECT_EQ(test_event->call_count(), 4u);

  test_event->wait(5);
  EXPECT_EQ(test_event->call_count(), 5u);

  periodic->disable();
  dispatcher->shutdown(true);
}

TEST(dds_DCPS_PeriodicEvent, NoDoubleExec)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::PeriodicEvent> periodic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::PeriodicEvent>(dispatcher, test_event);

  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, false);
  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, false);
  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, true);

  test_event->wait(1);
  EXPECT_EQ(test_event->call_count(), 1u);

  periodic->disable();

  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, true);
  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, true);
  periodic->enable(OpenDDS::DCPS::TimeDuration(0, 200000), false, false);

  test_event->wait(2);
  EXPECT_EQ(test_event->call_count(), 2u);

  periodic->disable();
  dispatcher->shutdown(true);
}
