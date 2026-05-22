/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/ThreadStatusManager.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Thread.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

namespace {
  class TestThreadStatusListener : public ThreadStatusListener {
  public:
    TestThreadStatusListener()
    : started_count_(0)
    , finished_count_(0)
    , cv_(lock_)
    {}

    void on_thread_started(const ThreadInfo& info)
    {
      ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
      ++started_count_;
      last_started_info_ = info;
      cv_.notify_all();
    }

    void on_thread_finished(const ThreadInfo& info)
    {
      ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
      ++finished_count_;
      last_finished_info_ = info;
      cv_.notify_all();
    }

    void wait_for_finished(unsigned count)
    {
      ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
      while (finished_count_ < count) {
        cv_.wait(tsm_);
      }
    }

    unsigned started_count_;
    unsigned finished_count_;
    ThreadInfo last_started_info_;
    ThreadInfo last_finished_info_;

  private:
    ACE_Thread_Mutex lock_;
    ConditionVariable<ACE_Thread_Mutex> cv_;
    ThreadStatusManager tsm_;
  };

  struct ThreadTestArgs {
    ThreadStatusManager* tsm;
    ACE_thread_t captured_handle;
  };

  ACE_THR_FUNC_RETURN test_thread_func(void* arg)
  {
    ThreadTestArgs* args = static_cast<ThreadTestArgs*>(arg);
    args->captured_handle = ACE_OS::thr_self();
    ThreadStatusManager::Start start(*args->tsm, "SeparateThread");
    return 0;
  }
}

TEST(dds_DCPS_ThreadStatusManager, ThreadStatusListener)
{
  ThreadStatusManager tsm;
  TestThreadStatusListener listener;
  tsm.set_thread_status_listener(&listener);

  EXPECT_EQ(listener.started_count_, 0u);
  EXPECT_EQ(listener.finished_count_, 0u);

  {
    ThreadStatusManager::Start start(tsm, "TestThread");
    EXPECT_EQ(listener.started_count_, 1u);
    EXPECT_EQ(listener.finished_count_, 0u);
    EXPECT_EQ(listener.last_started_info_.name, "TestThread");
  }

  EXPECT_EQ(listener.started_count_, 1u);
  EXPECT_EQ(listener.finished_count_, 1u);
  EXPECT_EQ(listener.last_finished_info_.name, "TestThread");
}

TEST(dds_DCPS_ThreadStatusManager, ThreadStatusListenerSeparateThread)
{
  ThreadStatusManager tsm;
  TestThreadStatusListener listener;
  tsm.set_thread_status_listener(&listener);

  ThreadTestArgs args;
  args.tsm = &tsm;
  args.captured_handle = 0;

  ACE_hthread_t thread_handle;
  ACE_Thread::spawn(test_thread_func, &args, THR_NEW_LWP | THR_JOINABLE, 0, &thread_handle);

  listener.wait_for_finished(1);
  ACE_Thread::join(thread_handle);

  EXPECT_EQ(listener.started_count_, 1u);
  EXPECT_EQ(listener.finished_count_, 1u);
  EXPECT_EQ(listener.last_started_info_.name, "SeparateThread");
  EXPECT_EQ(listener.last_finished_info_.name, "SeparateThread");
  EXPECT_EQ(listener.last_started_info_.handle, args.captured_handle);
  EXPECT_EQ(listener.last_finished_info_.handle, args.captured_handle);
}
