#include <dds/DCPS/Timers.h>

#include <ace/Reactor.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

struct TestEventHandler : RcEventHandler {
  TestEventHandler()
    : calls_(0)
  {}

  int handle_timeout(const ACE_Time_Value&, const void*)
  {
    ACE_DEBUG((LM_DEBUG, "%T timeout\n"));
    ++calls_;
    return 0;
  }

  int calls_;
};

TEST(dds_DCPS_Timers, test)
{
  RcHandle<TestEventHandler> handler = make_rch<TestEventHandler>();
  ACE_Reactor* const reactor = ACE_Reactor::instance();

  ACE_DEBUG((LM_DEBUG, "%T schedule\n"));
  const Timers::TimerId id = Timers::schedule(reactor, *handler, 0, TimeDuration::from_msec(10));
  ASSERT_NE(id, Timers::InvalidTimerId);

  ACE_Time_Value one_sec(1, 0);
  reactor->handle_events(one_sec);

  ASSERT_EQ(handler->calls_, 1);
  Timers::cancel(reactor, id);
}
