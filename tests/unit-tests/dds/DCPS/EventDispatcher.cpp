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

struct SimpleTestObj
{
  SimpleTestObj() : call_count_(0) {}

  //void operator()() { ++call_count_; std::stringstream ss; ss << "SimpleTestObj::operator() - call_count_ = " << call_count_.value() << std::endl; std::cout << ss.str() << std::flush; }
  void operator()() { ++call_count_; }

  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> call_count_;
};

TEST(dds_DCPS_EventDispatcher, SimpleDispatchAlpha)
{
  SimpleTestObj test_obj;
  OpenDDS::DCPS::EventDispatcher dispatcher;
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  while (test_obj.call_count_ != 3) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
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
  while (test_obj.call_count_ != 5) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
}

struct RecursiveTestObjOne
{
  RecursiveTestObjOne(OpenDDS::DCPS::EventDispatcher& dispatcher) : dispatcher_(dispatcher), call_count_(0) {}

  void operator()() { if (++call_count_ % 2) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::EventDispatcher& dispatcher_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> call_count_;
};

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchAlpha)
{
  OpenDDS::DCPS::EventDispatcher dispatcher;
  RecursiveTestObjOne test_obj(dispatcher);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  dispatcher.dispatch(test_obj);
  while (test_obj.call_count_ != 6) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
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
  while (test_obj.call_count_ != 10) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
}

struct RecursiveTestObjTwo
{
  RecursiveTestObjTwo(OpenDDS::DCPS::EventDispatcher& dispatcher, size_t dispatch_scale) : dispatcher_(dispatcher), call_count_(0), dispatch_scale_(dispatch_scale) {}

  void operator()() { ++call_count_; const size_t scale = dispatch_scale_.value(); for (size_t i = 0; i < scale; ++i) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::EventDispatcher& dispatcher_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> call_count_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> dispatch_scale_;
};

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchGamma)
{
  OpenDDS::DCPS::EventDispatcher dispatcher;
  RecursiveTestObjTwo test_obj(dispatcher, 1);
  dispatcher.dispatch(test_obj);
  while (test_obj.call_count_ < 1000) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
  test_obj.dispatch_scale_ = 0;
}

TEST(dds_DCPS_EventDispatcher, RecursiveDispatchDelta)
{
  OpenDDS::DCPS::EventDispatcher dispatcher(8);
  RecursiveTestObjTwo test_obj(dispatcher, 2);
  dispatcher.dispatch(test_obj);
  while (test_obj.call_count_ < 1000000) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
  test_obj.dispatch_scale_ = 0;
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
  ACE_OS::sleep(ACE_Time_Value(0, 800000));
  EXPECT_EQ(test_obj.call_count_, 6);
  while (test_obj.call_count_ != 12) {
    ACE_OS::sleep(ACE_Time_Value(0, 10000));
  }
}
