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

struct TestObj : public RcObject
{
  typedef PmfSporadicTask<TestObj> Sporadic;

  TestObj(const TimeSource& time_source,
          RcHandle<ReactorInterceptor> reactor_interceptor)
    : do_schedule_(false)
  {
    sporadic_ = make_rch<Sporadic>(time_source, reactor_interceptor, rchandle_from(this), &TestObj::execute);
  }

  void execute(const MonotonicTimePoint&) {
    ACE_DEBUG((LM_DEBUG, "TestObj::execute() called at %T\n"));
    ++total_count;
    if (do_schedule_.value()) {
      sporadic_->schedule(TimeDuration::from_msec(100)); // 0.1 seconds from now
    }
  }

  void set_do_schedule(bool do_schedule) { do_schedule_ = do_schedule; }

  RcHandle<Sporadic> sporadic_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> do_schedule_;
};

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  using namespace OpenDDS::DCPS;
  TimeSource time_source;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(0, &tsm);

  // Note: This test is modeled directly on the MultiTask stress test, which has a "fallback" timer
  // Since SporadicTask doesn't have this, we expect the number of total executions to be different

  RcHandle<TestObj> obj = make_rch<TestObj>(time_source, reactor_task.interceptor());
  obj->sporadic_->schedule(TimeDuration::from_msec(2000));
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 0);
  ACE_OS::sleep(5);
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 1);
  obj->set_do_schedule(true);
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration::from_msec(2000);
  size_t schedule_calls = 0;
  while (MonotonicTimePoint::now() < deadline) {
    ++schedule_calls;
    obj->sporadic_->schedule(TimeDuration::from_msec(100)); // 0.1 seconds from now
    ACE_OS::sleep(ACE_Time_Value(0, 1000)); // sleep for 0.001 seconds
  }
  obj->set_do_schedule(false);
  ACE_OS::sleep(ACE_Time_Value(0, 110000)); // sleep for 0.11 seconds to catch final "fast" executions
  ACE_DEBUG((LM_DEBUG, "schedule_calls = %d\n", schedule_calls));
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 21); // 1 from the slow period, 20 from the fast period (2.0 / 0.1)
  ACE_OS::sleep(2);
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 21); // No lingering enables / executions mean total count should be unchanged
  obj->sporadic_->cancel();

  reactor_task.stop();
  return 0;
}
