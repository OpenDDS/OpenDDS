/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/DispatchService.h>

#include <ace/OS_NS_unistd.h>

#include <gtest/gtest.h>

namespace {

class TestObjBase : public OpenDDS::DCPS::RcObject {
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

struct SimpleTestObj : public TestObjBase {
  SimpleTestObj() {}
  void operator()() { increment_call_count(); }
};

struct RecursiveTestObjOne : public TestObjBase {
  RecursiveTestObjOne(OpenDDS::DCPS::DispatchService& dispatcher) : dispatcher_(dispatcher) {}

  void operator()() { if (increment_call_count() % 2) { dispatcher_.dispatch(*this); } }

  OpenDDS::DCPS::DispatchService& dispatcher_;
};

struct RecursiveTestObjTwo : public TestObjBase {
  RecursiveTestObjTwo(OpenDDS::DCPS::DispatchService& dispatcher, size_t dispatch_scale) : dispatcher_(dispatcher), dispatch_scale_(dispatch_scale) {}

  void operator()()
  {
    increment_call_count();
    const size_t scale = dispatch_scale_.value();
    for (size_t i = 0; i < scale; ++i) {
      dispatcher_.dispatch(*this);
    }
  }

  OpenDDS::DCPS::DispatchService& dispatcher_;
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> dispatch_scale_;
};

struct InternalShutdownTestObj : public TestObjBase {
  InternalShutdownTestObj(OpenDDS::DCPS::DispatchService& ds) : ds_(ds) {}
  void operator()() { ds_.shutdown(true); }
  OpenDDS::DCPS::DispatchService& ds_;
};

} // (anonymous) namespace

TEST(dds_DCPS_DispatchService, RecursiveDispatchDelta)
{
  OpenDDS::DCPS::DispatchService dispatcher(8);
  RecursiveTestObjTwo test_obj(dispatcher, 2);

  dispatcher.dispatch(test_obj);

  test_obj.wait(100000u);
  test_obj.dispatch_scale_ = 0;
  dispatcher.shutdown();

  EXPECT_GE(test_obj.call_count(), 100000u);
}

TEST(dds_DCPS_DispatchService, RecursiveDispatchDelta_ImmediateShutdown)
{
  OpenDDS::DCPS::DispatchService dispatcher(8);
  RecursiveTestObjTwo test_obj(dispatcher, 2);

  dispatcher.dispatch(test_obj);

  test_obj.wait(100000u);
  test_obj.dispatch_scale_ = 0;
  OpenDDS::DCPS::DispatchService::EventQueue temp;
  dispatcher.shutdown(true, &temp);

  EXPECT_GE(test_obj.call_count(), 100000u);
}
