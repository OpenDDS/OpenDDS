#include <dds/DCPS/Timers.h>

#include <ace/Select_Reactor.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

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

TEST(dds_DCPS_Timers, test_oneshot)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);

  const Timers::TimerId id = Timers::schedule(&reactor, *handler, 0, TimeDuration::from_msec(10));
  ASSERT_NE(id, Timers::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  Timers::cancel(&reactor, id);
}

TEST(dds_DCPS_Timers, test_repeat)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);

  const Timers::TimerId id = Timers::schedule(&reactor, *handler, 0, TimeDuration(), TimeDuration::from_msec(10));
  ASSERT_NE(id, Timers::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.run_reactor_event_loop(one_sec);

  ASSERT_GT(handler->calls_, 10);
  Timers::cancel(&reactor, id);
}

TEST(dds_DCPS_Timers, test_immediate)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);

  const Timers::TimerId id = Timers::schedule(&reactor, *handler, 0, TimeDuration(), TimeDuration());
  ASSERT_NE(id, Timers::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  Timers::cancel(&reactor, id);
}

TEST(dds_DCPS_Timers, test_negative)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor reactor(new ACE_Select_Reactor, true);

  const Timers::TimerId id = Timers::schedule(&reactor, *handler, 0, TimeDuration(-23));
  ASSERT_NE(id, Timers::InvalidTimerId);

  ACE_Time_Value one_sec(1);
  reactor.handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  Timers::cancel(&reactor, id);
}
