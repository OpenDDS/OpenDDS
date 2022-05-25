/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_NS_unistd.h"

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/MultiTask.h"
#include "dds/DCPS/ReactorTask.h"

#include "../../../DCPS/common/TestSupport.h"

using namespace OpenDDS::DCPS;

ACE_Atomic_Op<ACE_Thread_Mutex, unsigned int> total_count = 0;

struct TestObj : public RcObject
{
  typedef PmfMultiTask<TestObj> Multi;

  TestObj(RcHandle<ReactorInterceptor> reactor_interceptor)
    : do_enable_(false)
  {
    multi_ = make_rch<Multi>(reactor_interceptor, TimeDuration::from_msec(2000), rchandle_from(this), &TestObj::execute);
  }

  void execute(const MonotonicTimePoint&) {
    ACE_DEBUG((LM_DEBUG, "TestObj::execute() called at %T\n"));
    ++total_count;
    if (do_enable_.value()) {
      multi_->enable(TimeDuration::from_msec(100)); // 0.1 seconds from now
    }
  }

  void set_do_enable(bool do_enable) { do_enable_ = do_enable; }

  RcHandle<Multi> multi_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> do_enable_;
};

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  using namespace OpenDDS::DCPS;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(0, &tsm);

  RcHandle<TestObj> obj = make_rch<TestObj>(reactor_task.interceptor());
  obj->multi_->enable(TimeDuration::from_msec(2000)); // 2.0 seconds
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 0);
  ACE_OS::sleep(5); // 5.0 seconds
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 2); // expect 2 executions within a 5.0 second interval when period is 2.0 seconds
  obj->set_do_enable(true);
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration::from_msec(2000); // 2.0 seconds from now
  size_t enable_calls = 0;
  while (MonotonicTimePoint::now() < deadline) {
    ++enable_calls;
    obj->multi_->enable(TimeDuration::from_msec(100)); // 0.1 seconds from now
    ACE_OS::sleep(ACE_Time_Value(0, 1000)); // sleep for 0.001 seconds
  }
  obj->set_do_enable(false);
  ACE_OS::sleep(ACE_Time_Value(0, 110000)); // sleep for 0.11 seconds to catch final "fast" executions
  ACE_DEBUG((LM_DEBUG, "enable_calls = %d\n", enable_calls));
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 22); // 2 from initial slow period, 20 from fast period (2.0 / 0.1)
  ACE_OS::sleep(5); // sleep for 5.0 more seconds, should fall back to 2.0 second default period
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 24); // 2 from initial slow period, 20 from fast period, 2 more from last slow period
  obj->multi_->disable();

  reactor_task.stop();
  return 0;
}
