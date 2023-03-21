/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/SporadicEvent.h>

#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/ServiceEventDispatcher.h>

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

} // (anonymous) namespace

TEST(dds_DCPS_SporadicEvent, ConstructDestruct)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::SporadicEvent> sporadic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(dispatcher, test_event);
}

TEST(dds_DCPS_SporadicEvent, Nominal)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::SporadicEvent> sporadic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(dispatcher, test_event);

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 1000));

  test_event->wait(1);
  EXPECT_EQ(test_event->call_count(), 1u);
}

TEST(dds_DCPS_SporadicEvent, MoveUp)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::SporadicEvent> sporadic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(dispatcher, test_event);

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(1, 0));
  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 1000));

  test_event->wait(1);
  EXPECT_EQ(test_event->call_count(), 1u);

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(1, 0));
  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 1000));

  test_event->wait(2);
  EXPECT_EQ(test_event->call_count(), 2u);
}

TEST(dds_DCPS_SporadicEvent, NoDoubleExec)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::SporadicEvent> sporadic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(dispatcher, test_event);

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));
  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));
  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));

  test_event->wait(1);
  EXPECT_EQ(test_event->call_count(), 1u);

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));
  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));
  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));

  test_event->wait(2);
  EXPECT_EQ(test_event->call_count(), 2u);
}

TEST(dds_DCPS_SporadicEvent, Cancel)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::SporadicEvent> sporadic = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(dispatcher, test_event);

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(1));
  sporadic->cancel();

  sporadic->schedule(OpenDDS::DCPS::TimeDuration(0, 100000));

  test_event->wait(1);
  EXPECT_EQ(test_event->call_count(), 1u);
}

