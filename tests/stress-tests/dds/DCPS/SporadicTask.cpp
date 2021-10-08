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

unsigned int total_count = 0;

struct TestObj : RcObject
{
  TestObj() : sporadic_(0), do_enable_(false) {}

  void execute(const MonotonicTimePoint&) {
    ACE_DEBUG((LM_DEBUG, "TestObj::execute() called at %T\n"));
    ++total_count;
    if (do_enable_) {
      sporadic_->schedule(TimeDuration::from_msec(100));
    }
  }

  void set_do_enable(bool do_enable) { do_enable_ = do_enable; }

  PmfSporadicTask<TestObj>* sporadic_;
  bool do_enable_;
};

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  using namespace OpenDDS::DCPS;
  ReactorTask reactor_task(false);
  reactor_task.open(0);
  TestObj obj;
  {
    PmfSporadicTask<TestObj> sporadic(reactor_task.interceptor(), obj, &TestObj::execute);
    obj.sporadic_ = &sporadic;
    sporadic.schedule(TimeDuration::from_msec(2000));
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count));
    TEST_CHECK(total_count == 0);
    ACE_OS::sleep(5);
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count));
    TEST_CHECK(total_count == 1);
    obj.set_do_enable(true);
    const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration::from_msec(2000);
    size_t enable_calls = 0;
    while (MonotonicTimePoint::now() < deadline) {
      ++enable_calls;
      sporadic.schedule(TimeDuration::from_msec(100));
      ACE_OS::sleep(ACE_Time_Value(0, 1000));
    }
    obj.set_do_enable(false);
    ACE_DEBUG((LM_DEBUG, "enable_calls = %d\n", enable_calls));
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count));
    TEST_CHECK(total_count >= 19);
    TEST_CHECK(total_count <= 21);
    const unsigned int prev_total_count = total_count;
    ACE_OS::sleep(5);
    ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count));
    TEST_CHECK(total_count = prev_total_count + 3);
    sporadic.cancel_and_wait();
  }
  reactor_task.stop();
  return 0;
}
