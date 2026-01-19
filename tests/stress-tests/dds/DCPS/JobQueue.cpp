/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/JobQueue.h>

#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/ReactorTask.h>
#include <dds/DCPS/ServiceEventDispatcher.h>

#include <gtest/gtest.h>

namespace {

OpenDDS::DCPS::ThreadStatusManager tsm;

class TestJob : public OpenDDS::DCPS::Job
{
  OpenDDS::DCPS::WeakRcHandle<OpenDDS::DCPS::JobQueue> job_queue_;
  const size_t count_max_;
  const size_t count_per_execute_;
  size_t current_count_;
  ACE_Thread_Mutex mutex_;
  OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> cv_;

public:
  TestJob(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::JobQueue> job_queue, size_t count_max, size_t count_per_execute)
    : job_queue_(job_queue)
    , count_max_(count_max)
    , count_per_execute_(count_per_execute)
    , current_count_(0)
    , cv_(mutex_)
  {}

  virtual ~TestJob()
  {
    wait_for_max_count();
  }

  void execute()
  {
    // static OpenDDS::DCPS::Atomic<size_t> execution_count(0);
    // const size_t execution = ++execution_count;
    ACE_Guard<ACE_Thread_Mutex> lock(mutex_);
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::JobQueue> queue = job_queue_.lock();
    if (queue) {
      // ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestJob::execute() - Execution %d: current_count_ %d\n"), execution, current_count_));
      for (size_t i = 0; ++current_count_ < count_max_ && i < count_per_execute_; ++i) {
        // ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestJob::execute() - Execution %d: current_count_ %d, i = %i\n"), execution, current_count_, i));
        queue->enqueue(rchandle_from(this));
      }
    }
    cv_.notify_all();
  }

  void wait_for_max_count()
  {
    ACE_Guard<ACE_Thread_Mutex> lock(mutex_);
    while (current_count_ < count_max_) {
      cv_.wait(tsm);
    }
  }

};

} // (anonymous) namespace

TEST(dds_DCPS_JobQueue, MaxInternalQueue)
{
  // OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::ReactorTask> reactor_task = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ReactorTask>();
  // reactor_task->open_reactor_task(&tsm, "JobQueue Stress Test");

  OpenDDS::DCPS::EventDispatcher_rch event_dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>(1);

  // OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::JobQueue> job_queue = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::JobQueue>(reactor_task->get_reactor());
  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::JobQueue> job_queue = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::JobQueue>(event_dispatcher);

  OpenDDS::DCPS::RcHandle<TestJob> test_job = OpenDDS::DCPS::make_rch<TestJob>(job_queue, 10000000, 1000);
  job_queue->enqueue(OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::Job>(test_job));

  test_job->wait_for_max_count();

  // reactor_task->close();
  event_dispatcher->shutdown(true);

  EXPECT_GE(0, 0);
}

TEST(dds_DCPS_JobQueue, MaxNotificationQueue)
{
  // OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::ReactorTask> reactor_task = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ReactorTask>();
  // reactor_task->open_reactor_task(&tsm, "JobQueue Stress Test");

  OpenDDS::DCPS::EventDispatcher_rch event_dispatcher = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ServiceEventDispatcher>(1);

  OPENDDS_VECTOR(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::JobQueue>) queue_vec;
  OPENDDS_VECTOR(OpenDDS::DCPS::RcHandle<TestJob>) job_vec;

  const size_t limit = 100000;

  for (size_t i = 0; i < limit; ++i) {
    // queue_vec.push_back(OpenDDS::DCPS::make_rch<OpenDDS::DCPS::JobQueue>(reactor_task->get_reactor()));
    queue_vec.push_back(OpenDDS::DCPS::make_rch<OpenDDS::DCPS::JobQueue>(event_dispatcher));
    job_vec.push_back(OpenDDS::DCPS::make_rch<TestJob>(queue_vec.back(), 100, 1));
  }

  for (size_t i = 0; i < limit; ++i) {
    queue_vec[i]->enqueue(job_vec[i]);
  }

  for (size_t i = 0; i < limit; ++i) {
    job_vec[i]->wait_for_max_count();
  }

  // reactor_task->close();
  event_dispatcher->shutdown(true);

  EXPECT_GE(0, 0);
}
