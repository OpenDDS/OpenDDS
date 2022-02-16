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
      multi_->enable(TimeDuration::from_msec(100));
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
  obj->multi_->enable(TimeDuration::from_msec(2000));
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 0);
  ACE_OS::sleep(5);
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == 2);
  obj->set_do_enable(true);
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration::from_msec(2000);
  size_t enable_calls = 0;
  while (MonotonicTimePoint::now() < deadline) {
    ++enable_calls;
    obj->multi_->enable(TimeDuration::from_msec(100));
    ACE_OS::sleep(ACE_Time_Value(0, 1000));
  }
  obj->set_do_enable(false);
  ACE_DEBUG((LM_DEBUG, "enable_calls = %d\n", enable_calls));
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count >= 19);
  TEST_CHECK(total_count <= 21);
  const unsigned int prev_total_count = total_count.value();
  ACE_OS::sleep(5);
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.value()));
  TEST_CHECK(total_count == prev_total_count + 3);
  obj->multi_->disable();

  reactor_task.stop();
  return 0;
}
