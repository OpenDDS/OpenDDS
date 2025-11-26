/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TimingChecker.h"

#include <dds/DCPS/Atomic.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/MultiTask.h>
#include <dds/DCPS/ReactorTask.h>

#include <ace/OS_NS_unistd.h>

#include <gtestWrapper.h>

using namespace OpenDDS::DCPS;

namespace {

OpenDDS::DCPS::Atomic<unsigned int> total_count(0u);

struct TestObj : public virtual RcObject
{
  typedef PmfMultiTask<TestObj> Multi;

  TestObj(RcHandle<ReactorTask> reactor_task)
    : do_enable_(false)
  {
    multi_ = make_rch<Multi>(reactor_task, TimeDuration::from_msec(2000), rchandle_from(this), &TestObj::execute);
  }

  void execute(const MonotonicTimePoint&) {
    ACE_DEBUG((LM_DEBUG, "TestObj::execute() called at %T\n"));
    ++total_count;
    if (do_enable_) {
      multi_->enable(TimeDuration::from_msec(100)); // 0.1 seconds from now
    }
  }

  void set_do_enable(bool do_enable) { do_enable_ = do_enable; }

  RcHandle<Multi> multi_;
  OpenDDS::DCPS::Atomic<bool> do_enable_;
};

} // (anonymous) namespace

TEST(dds_DCPS_MultiTask, TimingChecker)
{
  using namespace OpenDDS::DCPS;

  bool tight_timing = Utils::TimingChecker::check_timing();

  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm);

  RcHandle<TestObj> obj = make_rch<TestObj>(rchandle_from(&reactor_task));
  obj->multi_->enable(TimeDuration::from_msec(2000)); // 2.0 seconds
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  EXPECT_EQ(total_count, 0u);
  ACE_OS::sleep(5); // 5.0 seconds
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  EXPECT_EQ(total_count, 2u); // expect 2 executions within a 5.0 second interval when period is 2.0 seconds
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
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  // 1 from the slow period, 20 from the fast period (2.0 / 0.1)
  if (tight_timing) {
    EXPECT_EQ(total_count, 22u);
  } else {
    EXPECT_GE(total_count, 17u);
    EXPECT_LE(total_count, 22u);
  }
  ACE_OS::sleep(5); // sleep for 5.0 more seconds, should fall back to 2.0 second default period
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  // 1 from the slow period, 20 from the fast period (2.0 / 0.1), 2 more from last slow period
  if (tight_timing) {
    EXPECT_EQ(total_count, 24u);
  } else {
    EXPECT_GE(total_count, 19u);
    EXPECT_LE(total_count, 24u);
  }
  obj->multi_->disable();

  reactor_task.stop();
}
