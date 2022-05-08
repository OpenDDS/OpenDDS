/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/EventDispatcherLite.h>

#include <ace/OS_NS_unistd.h>

#include <gtest/gtest.h>

namespace
{

class TestObjBase : public OpenDDS::DCPS::RcObject
{
public:
  TestObjBase() : cv_(mutex_), call_count_(0) {}

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

struct SimpleTestObj : public TestObjBase
{
  SimpleTestObj() {}
  void operator()() { increment_call_count(); }
};

struct RecursiveTestObjOne : public TestObjBase
{
  RecursiveTestObjOne(OpenDDS::DCPS::EventDispatcherLite& dispatcher) : dispatcher_(dispatcher) {}

  void operator()() { if (increment_call_count() % 2) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::EventDispatcherLite& dispatcher_;
};

struct RecursiveTestObjTwo : public TestObjBase
{
  RecursiveTestObjTwo(OpenDDS::DCPS::EventDispatcherLite& dispatcher, size_t dispatch_scale) : dispatcher_(dispatcher), dispatch_scale_(dispatch_scale) {}

  void operator()() { increment_call_count(); const size_t scale = dispatch_scale_.value(); for (size_t i = 0; i < scale; ++i) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::EventDispatcherLite& dispatcher_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> dispatch_scale_;
};

} // (anonymous) namespace

TEST(dds_DCPS_EventDispatcherLite, DefaultConstructor)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher;
}

TEST(dds_DCPS_EventDispatcherLite, ArgConstructorFour)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher(4);
}

TEST(dds_DCPS_EventDispatcherLite, ArgConstructorOrderAlpha)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher_four(4);
  OpenDDS::DCPS::EventDispatcherLite dispatcher_two(2);
}

TEST(dds_DCPS_EventDispatcherLite, ArgConstructorOrderBeta)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher_four(2);
  OpenDDS::DCPS::EventDispatcherLite dispatcher_two(4);
}

TEST(dds_DCPS_EventDispatcherLite, SimpleDispatchAlpha)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcherLite dispatcher;

  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(3u);
  EXPECT_EQ(test_obj.call_count(), 3u);
}

TEST(dds_DCPS_EventDispatcherLite, SimpleDispatchBeta)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcherLite dispatcher(8);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(5u);
  EXPECT_EQ(test_obj.call_count(), 5u);
}

TEST(dds_DCPS_EventDispatcherLite, RecursiveDispatchAlpha)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher;
  RecursiveTestObjOne test_obj(dispatcher);

  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(6u);
  dispatcher.shutdown();

  EXPECT_GE(test_obj.call_count(), 6u);
}

TEST(dds_DCPS_EventDispatcherLite, RecursiveDispatchBeta)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher(8);
  RecursiveTestObjOne test_obj(dispatcher);

  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(10u);
  dispatcher.shutdown();

  EXPECT_GE(test_obj.call_count(), 10u);
}

TEST(dds_DCPS_EventDispatcherLite, RecursiveDispatchGamma)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher;
  RecursiveTestObjTwo test_obj(dispatcher, 1);

  dispatcher.dispatch(test_obj);

  test_obj.wait(1000u);
  test_obj.dispatch_scale_ = 0;
  dispatcher.shutdown();

  EXPECT_GE(test_obj.call_count(), 1000u);
}

TEST(dds_DCPS_EventDispatcherLite, RecursiveDispatchDelta)
{
  OpenDDS::DCPS::EventDispatcherLite dispatcher(8);
  RecursiveTestObjTwo test_obj(dispatcher, 2);

  dispatcher.dispatch(test_obj);

  test_obj.wait(100000u);
  test_obj.dispatch_scale_ = 0;
  dispatcher.shutdown();

  EXPECT_GE(test_obj.call_count(), 100000u);
}

TEST(dds_DCPS_EventDispatcherLite, TimedDispatch)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcherLite dispatcher(4);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));
  dispatcher.dispatch(test_obj);

  test_obj.wait(6u);
  const OpenDDS::DCPS::MonotonicTimePoint after6 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(7u);
  const OpenDDS::DCPS::MonotonicTimePoint after7 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(8u);
  const OpenDDS::DCPS::MonotonicTimePoint after8 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(9u);
  const OpenDDS::DCPS::MonotonicTimePoint after9 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(10u);
  const OpenDDS::DCPS::MonotonicTimePoint after10 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(11u);
  const OpenDDS::DCPS::MonotonicTimePoint after11 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(12u);
  const OpenDDS::DCPS::MonotonicTimePoint after12 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_LT(now, after6);
  EXPECT_LT(after6, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));

  EXPECT_GE(after7, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));
  EXPECT_GE(after8, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  EXPECT_GE(after9, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  EXPECT_GE(after10, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  EXPECT_GE(after11, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  EXPECT_GE(after12, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
}

TEST(dds_DCPS_EventDispatcherLite, TimedDispatchSingleThreaded)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcherLite dispatcher(1);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));
  dispatcher.dispatch(test_obj);

  test_obj.wait(6u);
  const OpenDDS::DCPS::MonotonicTimePoint after6 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(7u);
  const OpenDDS::DCPS::MonotonicTimePoint after7 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(8u);
  const OpenDDS::DCPS::MonotonicTimePoint after8 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(9u);
  const OpenDDS::DCPS::MonotonicTimePoint after9 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(10u);
  const OpenDDS::DCPS::MonotonicTimePoint after10 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(11u);
  const OpenDDS::DCPS::MonotonicTimePoint after11 = OpenDDS::DCPS::MonotonicTimePoint::now();

  test_obj.wait(12u);
  const OpenDDS::DCPS::MonotonicTimePoint after12 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_LT(now, after6);
  EXPECT_LT(after6, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));

  EXPECT_GE(after7, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));
  EXPECT_GE(after8, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  EXPECT_GE(after9, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  EXPECT_GE(after10, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  EXPECT_GE(after11, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  EXPECT_GE(after12, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
}

TEST(dds_DCPS_EventDispatcherLite, CancelDispatch)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcherLite dispatcher(4);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));

  EXPECT_EQ(dispatcher.cancel(test_obj), 6u);

  long t1 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
  long t2 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  long t3 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  /*long t4 =*/ dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  /*long t5 =*/ dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  long t6 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));

  EXPECT_EQ(dispatcher.cancel(t6), 1u);
  EXPECT_EQ(dispatcher.cancel(t1), 1u);
  EXPECT_EQ(dispatcher.cancel(t2), 1u);
  EXPECT_EQ(dispatcher.cancel(t3), 1u);

  test_obj.wait(2u);
  const OpenDDS::DCPS::MonotonicTimePoint after2 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_GE(after2, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  EXPECT_EQ(test_obj.call_count(), 2u);
}

TEST(dds_DCPS_EventDispatcherLite, CancelDispatchSingleThreaded)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcherLite dispatcher(1);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));

  EXPECT_EQ(dispatcher.cancel(test_obj), 6u);

  long t1 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.9));
  long t2 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.5));
  long t3 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.8));
  /*long t4 =*/ dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  /*long t5 =*/ dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.6));
  long t6 = dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(0.4));

  EXPECT_EQ(dispatcher.cancel(t6), 1u);
  EXPECT_EQ(dispatcher.cancel(t1), 1u);
  EXPECT_EQ(dispatcher.cancel(t2), 1u);
  EXPECT_EQ(dispatcher.cancel(t3), 1u);

  test_obj.wait(2u);
  const OpenDDS::DCPS::MonotonicTimePoint after2 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_GE(after2, now + OpenDDS::DCPS::TimeDuration::from_double(0.7));
  EXPECT_EQ(test_obj.call_count(), 2u);
}
