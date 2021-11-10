/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_NS_unistd.h"

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/SporadicTask.h"
#include "dds/DCPS/ReactorTask.h"

#include "../../../DCPS/common/TestSupport.h"

using namespace OpenDDS::DCPS;

ACE_Atomic_Op<ACE_Thread_Mutex, unsigned int> total_count = 0;

struct TestObj : RcObject
{
  TestObj() : sporadic_(0), do_enable_(false) {}

  void execute(const MonotonicTimePoint&) {
    ACE_DEBUG((LM_DEBUG, "TestObj::execute() called at %T\n"));
    ++total_count;
    if (do_enable_.value()) {
      sporadic_->schedule(TimeDuration::from_msec(100));
    }
  }

  void set_do_enable(bool do_enable) { do_enable_ = do_enable; }

  PmfSporadicTask<TestObj>* sporadic_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> do_enable_;
};

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  using namespace OpenDDS::DCPS;
  ReactorTask reactor_task(false);
  reactor_task.open(0);
  TestObj obj;
  {
    TimeSource time_source;
    PmfSporadicTask<TestObj> sporadic(time_source, reactor_task.interceptor(), obj, &TestObj::execute);
    obj.sporadic_ = &sporadic;
    sporadic.schedule(TimeDuration::from_msec(2000));
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
    TEST_CHECK(total_count == 0);
    ACE_OS::sleep(5);
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
    TEST_CHECK(total_count == 1);
    obj.set_do_enable(true);
    const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration::from_msec(2000);
    size_t schedule_calls = 0;
    while (MonotonicTimePoint::now() < deadline) {
      ++schedule_calls;
      sporadic.schedule(TimeDuration::from_msec(100));
      ACE_OS::sleep(ACE_Time_Value(0, 1000));
    }
    obj.set_do_enable(false);
    ACE_DEBUG((LM_DEBUG, "schedule_calls = %d\n", schedule_calls));
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
    TEST_CHECK(total_count >= 19);
    TEST_CHECK(total_count <= 21);
    const unsigned int prev_total_count = total_count.value();
    ACE_OS::sleep(5);
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
    TEST_CHECK(total_count == prev_total_count + 1);
    sporadic.cancel_and_wait();
  }
  reactor_task.stop();
  return 0;
}
