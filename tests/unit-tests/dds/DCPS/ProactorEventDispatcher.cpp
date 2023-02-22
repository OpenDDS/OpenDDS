/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/ProactorEventDispatcher.h>

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

TEST(dds_DCPS_ProactorEventDispatcher, DefaultConstructor)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();
}

TEST(dds_DCPS_ProactorEventDispatcher, ArgConstructorFour)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(4);
}

TEST(dds_DCPS_ProactorEventDispatcher, ArgConstructorOrderAlpha)
{
  {
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher_four = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(4);
  }
  {
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher_two = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(2);
  }
}

TEST(dds_DCPS_ProactorEventDispatcher, ArgConstructorOrderBeta)
{
  {
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher_two = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(2);
  }
  {
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher_four = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(4);
  }
}

TEST(dds_DCPS_ProactorEventDispatcher, SimpleDispatchAlpha)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();

  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);

  test_event->wait(3u);
  dispatcher->shutdown();

  EXPECT_EQ(test_event->call_count(), 3u);
}

TEST(dds_DCPS_ProactorEventDispatcher, SimpleDispatchBeta)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(8);

  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);

  test_event->wait(5u);
  dispatcher->shutdown();

  EXPECT_EQ(test_event->call_count(), 5u);
}

TEST(dds_DCPS_ProactorEventDispatcher, RecursiveDispatchAlpha)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();
  OpenDDS::DCPS::RcHandle<RecursiveTestEventOne> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventOne>(dispatcher);

  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);

  test_event->wait(6u);
  dispatcher->shutdown();

  EXPECT_GE(test_event->call_count(), 6u);
}

TEST(dds_DCPS_ProactorEventDispatcher, RecursiveDispatchAlpha_ImmediateShutdown)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();
  OpenDDS::DCPS::RcHandle<RecursiveTestEventOne> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventOne>(dispatcher);

  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);

  test_event->wait(6u);
  dispatcher->shutdown(true);

  EXPECT_GE(test_event->call_count(), 6u);
}

TEST(dds_DCPS_ProactorEventDispatcher, RecursiveDispatchBeta)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(8);
  OpenDDS::DCPS::RcHandle<RecursiveTestEventOne> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventOne>(dispatcher);

  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);

  test_event->wait(10u);
  dispatcher->shutdown();

  EXPECT_GE(test_event->call_count(), 10u);
}

TEST(dds_DCPS_ProactorEventDispatcher, RecursiveDispatchBeta_ImmediateShutdown)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(8);
  OpenDDS::DCPS::RcHandle<RecursiveTestEventOne> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventOne>(dispatcher);

  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);
  dispatcher->dispatch(test_event);

  test_event->wait(10u);
  dispatcher->shutdown(true);

  EXPECT_GE(test_event->call_count(), 10u);
}

TEST(dds_DCPS_ProactorEventDispatcher, RecursiveDispatchGamma)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();
  OpenDDS::DCPS::RcHandle<RecursiveTestEventTwo> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventTwo>(dispatcher, 1);

  dispatcher->dispatch(test_event);

  test_event->wait(1000u);
  test_event->dispatch_scale_ = 0;
  dispatcher->shutdown();

  EXPECT_GE(test_event->call_count(), 1000u);
}

TEST(dds_DCPS_ProactorEventDispatcher, RecursiveDispatchGamma_ImmediateShutdown)
{
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();
  OpenDDS::DCPS::RcHandle<RecursiveTestEventTwo> test_event = OpenDDS::DCPS::make_rch<RecursiveTestEventTwo>(dispatcher, 1);

  dispatcher->dispatch(test_event);

  test_event->wait(1000u);
  test_event->dispatch_scale_ = 0;
  dispatcher->shutdown(true);

  EXPECT_GE(test_event->call_count(), 1000u);
}

TEST(dds_DCPS_ProactorEventDispatcher, TestShutdown)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>();

  dispatcher->dispatch(test_event);

  test_event->wait(1u);
  dispatcher->shutdown();

  EXPECT_FALSE(dispatcher->dispatch(test_event));
  EXPECT_EQ(-1, dispatcher->schedule(test_event, OpenDDS::DCPS::MonotonicTimePoint::now()));
  EXPECT_EQ(0u, dispatcher->cancel(1));

  EXPECT_EQ(test_event->call_count(), 1u);
}

TEST(dds_DCPS_ProactorEventDispatcher, TimedDispatch)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(4);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.09));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.05));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.08));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.06));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));
  dispatcher->dispatch(test_event);

  test_event->wait(6u);
  const OpenDDS::DCPS::MonotonicTimePoint after6 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(7u);
  const OpenDDS::DCPS::MonotonicTimePoint after7 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(8u);
  const OpenDDS::DCPS::MonotonicTimePoint after8 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(9u);
  const OpenDDS::DCPS::MonotonicTimePoint after9 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(10u);
  const OpenDDS::DCPS::MonotonicTimePoint after10 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(11u);
  const OpenDDS::DCPS::MonotonicTimePoint after11 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(12u);
  const OpenDDS::DCPS::MonotonicTimePoint after12 = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher->shutdown();

  EXPECT_LT(now, after6);
  EXPECT_LT(after6, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));

  EXPECT_GE(after7, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));
  EXPECT_GE(after8, now + OpenDDS::DCPS::TimeDuration::from_double(0.05));
  EXPECT_GE(after9, now + OpenDDS::DCPS::TimeDuration::from_double(0.06));
  EXPECT_GE(after10, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  EXPECT_GE(after11, now + OpenDDS::DCPS::TimeDuration::from_double(0.08));
  EXPECT_GE(after12, now + OpenDDS::DCPS::TimeDuration::from_double(0.09));
}

TEST(dds_DCPS_ProactorEventDispatcher, TimedDispatchSingleThreaded)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(1);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.09));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.05));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.08));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.06));
  dispatcher->dispatch(test_event);
  dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));
  dispatcher->dispatch(test_event);

  test_event->wait(6u);
  const OpenDDS::DCPS::MonotonicTimePoint after6 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(7u);
  const OpenDDS::DCPS::MonotonicTimePoint after7 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(8u);
  const OpenDDS::DCPS::MonotonicTimePoint after8 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(9u);
  const OpenDDS::DCPS::MonotonicTimePoint after9 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(10u);
  const OpenDDS::DCPS::MonotonicTimePoint after10 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(11u);
  const OpenDDS::DCPS::MonotonicTimePoint after11 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_event->wait(12u);
  const OpenDDS::DCPS::MonotonicTimePoint after12 = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher->shutdown();

  EXPECT_LT(now, after6);
  EXPECT_LT(after6, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));

  EXPECT_GE(after7, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));
  EXPECT_GE(after8, now + OpenDDS::DCPS::TimeDuration::from_double(0.05));
  EXPECT_GE(after9, now + OpenDDS::DCPS::TimeDuration::from_double(0.06));
  EXPECT_GE(after10, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  EXPECT_GE(after11, now + OpenDDS::DCPS::TimeDuration::from_double(0.08));
  EXPECT_GE(after12, now + OpenDDS::DCPS::TimeDuration::from_double(0.09));
}

TEST(dds_DCPS_ProactorEventDispatcher, CancelDispatch)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(4);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  long t1 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.09));
  long t2 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.05));
  long t3 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.08));
  /*long t4 =*/ dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  /*long t5 =*/ dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.06));
  long t6 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));

  EXPECT_EQ(dispatcher->cancel(t6), 1u);
  EXPECT_EQ(dispatcher->cancel(t1), 1u);
  EXPECT_EQ(dispatcher->cancel(t2), 1u);
  EXPECT_EQ(dispatcher->cancel(t3), 1u);

  test_event->wait(2u);
  const OpenDDS::DCPS::MonotonicTimePoint after2 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_GE(after2, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  EXPECT_EQ(test_event->call_count(), 2u);
}

TEST(dds_DCPS_ProactorEventDispatcher, CancelDispatchSingleThreaded)
{
  OpenDDS::DCPS::RcHandle<SimpleTestEvent> test_event = OpenDDS::DCPS::make_rch<SimpleTestEvent>();
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::EventDispatcher> dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ProactorEventDispatcher>(1);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  long t1 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.09));
  long t2 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.05));
  long t3 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.08));
  /*long t4 =*/ dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  /*long t5 =*/ dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.06));
  long t6 = dispatcher->schedule(test_event, now + OpenDDS::DCPS::TimeDuration::from_double(0.04));

  EXPECT_EQ(dispatcher->cancel(t6), 1u);
  EXPECT_EQ(dispatcher->cancel(t1), 1u);
  EXPECT_EQ(dispatcher->cancel(t2), 1u);
  EXPECT_EQ(dispatcher->cancel(t3), 1u);

  test_event->wait(2u);
  const OpenDDS::DCPS::MonotonicTimePoint after2 = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher->shutdown();

  EXPECT_GE(after2, now + OpenDDS::DCPS::TimeDuration::from_double(0.07));
  EXPECT_EQ(test_event->call_count(), 2u);
}
