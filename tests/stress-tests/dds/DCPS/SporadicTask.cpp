/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TimingChecker.h"

#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/SporadicTask.h>
#include <dds/DCPS/ReactorTask.h>

#include <ace/OS_NS_unistd.h>

#include <gtestWrapper.h>

using namespace OpenDDS::DCPS;

namespace {

OpenDDS::DCPS::Atomic<unsigned int> total_count(0u);

struct TestObj : public virtual RcObject
{
  typedef PmfSporadicTask<TestObj> Sporadic;

  TestObj(const TimeSource& time_source,
          RcHandle<ReactorTask> reactor_task)
    : do_schedule_(false)
  {
    sporadic_ = make_rch<Sporadic>(time_source, reactor_task, rchandle_from(this), &TestObj::execute);
  }

  void execute(const MonotonicTimePoint&) {
    ACE_DEBUG((LM_DEBUG, "TestObj::execute() called at %T\n"));
    ++total_count;
    if (do_schedule_) {
      sporadic_->schedule(TimeDuration::from_msec(100)); // 0.1 seconds from now
    }
  }

  void set_do_schedule(bool do_schedule) { do_schedule_ = do_schedule; }

  RcHandle<Sporadic> sporadic_;
  OpenDDS::DCPS::Atomic<bool> do_schedule_;
};

} // (anonymous) namespace

TEST(dds_DCPS_SporadicTask, TimingChecker)
{
  using namespace OpenDDS::DCPS;

  bool tight_timing = Utils::TimingChecker::check_timing();

  TimeSource time_source;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm);

  // Note: This test is modeled directly on the MultiTask stress test, which has a "fallback" timer
  // Since SporadicTask doesn't have this, we expect the number of total executions to be different

  RcHandle<TestObj> obj = make_rch<TestObj>(time_source, rchandle_from(&reactor_task));
  obj->sporadic_->schedule(TimeDuration::from_msec(2000));
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  EXPECT_EQ(total_count, 0u);
  ACE_OS::sleep(5);
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  EXPECT_EQ(total_count, 1u);
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
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  // 1 from the slow period, 20 from the fast period (2.0 / 0.1)
  if (tight_timing) {
    EXPECT_EQ(total_count, 21u);
  } else {
    EXPECT_GE(total_count, 16u);
    EXPECT_LE(total_count, 21u);
  }
  ACE_OS::sleep(2);
  ACE_DEBUG((LM_DEBUG, "total_count = %d\n", total_count.load()));
  // No lingering enables / executions mean total count should be unchanged
  if (tight_timing) {
    EXPECT_EQ(total_count, 21u);
  } else {
    EXPECT_GE(total_count, 16u);
    EXPECT_LE(total_count, 21u);
  }
  obj->sporadic_->cancel();

  reactor_task.stop();
}
