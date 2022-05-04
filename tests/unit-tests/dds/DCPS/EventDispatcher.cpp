/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/EventDispatcher.h>

#include <ace/OS_NS_unistd.h>

#include <gtest/gtest.h>

TEST(dds_DCPS_EventDispatcher, DefaultConstructor)
{
  OpenDDS::DCPS::EventDispatcher dispatcher;
}

TEST(dds_DCPS_EventDispatcher, ArgConstructorFour)
{
  OpenDDS::DCPS::EventDispatcher dispatcher(4);
}

TEST(dds_DCPS_EventDispatcher, ArgConstructorOrderAlpha)
{
  OpenDDS::DCPS::EventDispatcher dispatcher_four(4);
  OpenDDS::DCPS::EventDispatcher dispatcher_two(2);
}

TEST(dds_DCPS_EventDispatcher, ArgConstructorOrderBeta)
{
  OpenDDS::DCPS::EventDispatcher dispatcher_four(2);
  OpenDDS::DCPS::EventDispatcher dispatcher_two(4);
}

class TestObjBase
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

TEST(dds_DCPS_EventDispatcher, SimpleDispatchAlpha)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher;

  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(3u);
  EXPECT_EQ(test_obj.call_count(), 3u);
}

TEST(dds_DCPS_EventDispatcher, SimpleDispatchBeta)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher(8);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(5u);
  EXPECT_EQ(test_obj.call_count(), 5u);
}

struct RecursiveTestObjOne : public TestObjBase
{
  RecursiveTestObjOne(OpenDDS::DCPS::EventDispatcher& dispatcher) : dispatcher_(dispatcher) {}

  void operator()() { if (increment_call_count() % 2) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::EventDispatcher& dispatcher_;
};

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchAlpha)
{
  OpenDDS::DCPS::EventDispatcher dispatcher;
  RecursiveTestObjOne test_obj(dispatcher);

  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(6u);
  EXPECT_GE(test_obj.call_count(), 6u);
}

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchBeta)
{
  OpenDDS::DCPS::EventDispatcher dispatcher(8);
  RecursiveTestObjOne test_obj(dispatcher);

  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);

  test_obj.wait(10u);
  EXPECT_GE(test_obj.call_count(), 10u);
}

struct RecursiveTestObjTwo : public TestObjBase
{
  RecursiveTestObjTwo(OpenDDS::DCPS::EventDispatcher& dispatcher, size_t dispatch_scale) : dispatcher_(dispatcher), dispatch_scale_(dispatch_scale) {}

  void operator()() { increment_call_count(); const size_t scale = dispatch_scale_.value(); for (size_t i = 0; i < scale; ++i) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::EventDispatcher& dispatcher_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> dispatch_scale_;
};

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchGamma)
{
  OpenDDS::DCPS::EventDispatcher dispatcher;
  RecursiveTestObjTwo test_obj(dispatcher, 1);

  dispatcher.dispatch(test_obj);

  test_obj.wait(1000u);
  test_obj.dispatch_scale_ = 0;
  EXPECT_GE(test_obj.call_count(), 1000u);
}

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchDelta)
{
  OpenDDS::DCPS::EventDispatcher dispatcher(8);
  RecursiveTestObjTwo test_obj(dispatcher, 2);

  dispatcher.dispatch(test_obj);

  test_obj.wait(1000000u);
  test_obj.dispatch_scale_ = 0;
  EXPECT_GE(test_obj.call_count(), 1000000u);
}

TEST(dds_DCPS_EventDispatcher, TimedDispatch)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher(4);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));
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
  EXPECT_LT(after6, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));

  EXPECT_GE(after7, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));
  EXPECT_GE(after8, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  EXPECT_GE(after9, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  EXPECT_GE(after10, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  EXPECT_GE(after11, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  EXPECT_GE(after12, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
}

TEST(dds_DCPS_EventDispatcher, TimedDispatchSingleThreaded)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher(1);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  dispatcher.dispatch(test_obj);
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));
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
  EXPECT_LT(after6, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));

  EXPECT_GE(after7, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));
  EXPECT_GE(after8, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  EXPECT_GE(after9, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  EXPECT_GE(after10, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  EXPECT_GE(after11, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  EXPECT_GE(after12, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
}

TEST(dds_DCPS_EventDispatcher, CancelDispatch)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher(4);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));

  EXPECT_EQ(dispatcher.cancel(test_obj), 6u);

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));

  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0)), 1u);
  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66)), 1u);
  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33)), 1u);
  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33)), 1u);

  test_obj.wait(2u);
  const OpenDDS::DCPS::MonotonicTimePoint after2 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_GE(after2, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  EXPECT_EQ(test_obj.call_count(), 2u);
}

TEST(dds_DCPS_EventDispatcher, CancelDispatchSingleThreaded)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher(1);

  const OpenDDS::DCPS::MonotonicTimePoint now = OpenDDS::DCPS::MonotonicTimePoint::now();

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));

  EXPECT_EQ(dispatcher.cancel(test_obj), 6u);

  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.66));
  dispatcher.schedule(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0));

  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.0)), 1u);
  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.66)), 1u);
  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(1.33)), 1u);
  EXPECT_EQ(dispatcher.cancel(test_obj, now + OpenDDS::DCPS::TimeDuration::from_double(2.33)), 1u);

  test_obj.wait(2u);
  const OpenDDS::DCPS::MonotonicTimePoint after2 = OpenDDS::DCPS::MonotonicTimePoint::now();

  EXPECT_GE(after2, now + OpenDDS::DCPS::TimeDuration::from_double(2.0));
  EXPECT_EQ(test_obj.call_count(), 2u);
}
