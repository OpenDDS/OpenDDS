#include <gtestWrapper.h>

#include <dds/DCPS/ReactorTask.h>

#include <dds/DCPS/TimeSource.h>

#include <ace/Select_Reactor.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

namespace {
  class MyTimeSource : public TimeSource {
  public:
    MOCK_CONST_METHOD0(monotonic_time_point_now, MonotonicTimePoint());
  };

  class MyReactor : public ACE_Reactor {
  public:
    MOCK_METHOD4(schedule_timer, long(ACE_Event_Handler*, const void*, const ACE_Time_Value&, const ACE_Time_Value&));
    MOCK_METHOD3(cancel_timer, int(long, const void**, int));
    MOCK_METHOD3(cancel_timer, int(ACE_Event_Handler*, int));
  };

  struct TestEventHandler : RcEventHandler {
    TestEventHandler()
      : calls_(0)
    {}

    int handle_timeout(const ACE_Time_Value&, const void*)
    {
      ++calls_;
      return 0;
    }

    int calls_;
  };
}

TEST(dds_DCPS_ReactorWrapper, test_oneshot)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);
  ReactorWrapper reactor_wrapper;
  reactor_wrapper.open(&reactor);

  const ReactorWrapper::TimerId id = reactor_wrapper.schedule(*handler, 0, TimeDuration::from_msec(10));
  ASSERT_NE(id, ReactorWrapper::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  reactor_wrapper.cancel(id);
  reactor_wrapper.close();
}

TEST(dds_DCPS_ReactorWrapper, test_repeat)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);
  ReactorWrapper reactor_wrapper;
  reactor_wrapper.open(&reactor);

  const ReactorWrapper::TimerId id = reactor_wrapper.schedule(*handler, 0, TimeDuration(), TimeDuration::from_msec(10));
  ASSERT_NE(id, ReactorWrapper::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.run_reactor_event_loop(one_sec);

  ASSERT_GT(handler->calls_, 11);
  reactor_wrapper.cancel(id);
  reactor_wrapper.close();
}

TEST(dds_DCPS_ReactorWrapper, test_immediate)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);
  ReactorWrapper reactor_wrapper;
  reactor_wrapper.open(&reactor);

  const ReactorWrapper::TimerId id = reactor_wrapper.schedule(*handler, 0, TimeDuration(), TimeDuration());
  ASSERT_NE(id, ReactorWrapper::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  reactor_wrapper.cancel(id);
  reactor_wrapper.close();
}

TEST(dds_DCPS_ReactorWrapper, test_negative)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);
  ReactorWrapper reactor_wrapper;
  reactor_wrapper.open(&reactor);

  const ReactorWrapper::TimerId id = reactor_wrapper.schedule(*handler, 0, TimeDuration(-23));
  ASSERT_NE(id, ReactorWrapper::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  reactor_wrapper.cancel(id);
  reactor_wrapper.close();
}
