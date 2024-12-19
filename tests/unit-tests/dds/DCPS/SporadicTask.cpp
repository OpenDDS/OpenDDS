/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtestWrapper.h>

#include <dds/DCPS/SporadicTask.h>

#include "MockLogger.h"

#include <ace/Thread_Manager.h>

using namespace OpenDDS::DCPS;

#if !OPENDDS_CONFIG_BOOTTIME_TIMERS

namespace {
  class MyTimeSource : public TimeSource {
  public:
    MOCK_CONST_METHOD0(monotonic_time_point_now, MonotonicTimePoint());
  };

  class MyReactor : public ACE_Reactor {
  public:
    MOCK_METHOD4(schedule_timer, long(ACE_Event_Handler*, const void*, const ACE_Time_Value&, const ACE_Time_Value&));
    MOCK_METHOD3(cancel_timer, int(long, const void**, int));
  };

  class MySporadicTask : public SporadicTask {
  public:
    MySporadicTask(const TimeSource& time_source,
                   RcHandle<ReactorTask> reactor_task)
      : SporadicTask(time_source, reactor_task)
    {}
    long timer_id() const { return get_timer_id(); }

    MOCK_METHOD1(execute, void(const MonotonicTimePoint&));
  };

  class MyTestClass : public virtual RcObject {
  public:
    MyTestClass(const TimeSource& time_source,
                RcHandle<ReactorTask> reactor_task)
    {
      sporadic_task_ = make_rch<PmfSporadicTask<MyTestClass> >(time_source, reactor_task, rchandle_from(this), &MyTestClass::myfunc);
    }

    MOCK_METHOD1(myfunc, void(const MonotonicTimePoint&));
    RcHandle<SporadicTask> sporadic_task_;
  };
}

TEST(dds_DCPS_SporadicTask, schedule)
{
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, rchandle_from(&reactor_task));
  const TimeDuration one_second(1);
  const ACE_Time_Value now(7);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(1)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));
  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));
  EXPECT_CALL(*sporadic_task.get(), execute(MonotonicTimePoint(now)));

  sporadic_task->schedule(one_second);
  reactor_task.wait_until_empty();

  // Execute.
  RcHandle<RcEventHandler> eh = sporadic_task;
  eh->handle_timeout(now);

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, schedule_pmf)
{
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  MyTestClass mtc(time_source, rchandle_from(&reactor_task));
  const TimeDuration one_second(1);
  const ACE_Time_Value now(7);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(1)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));
  EXPECT_CALL(*reactor, schedule_timer(mtc.sporadic_task_.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));
  EXPECT_CALL(mtc, myfunc(MonotonicTimePoint(now)));

  mtc.sporadic_task_->schedule(one_second);
  reactor_task.wait_until_empty();

  // Execute.
  RcEventHandler* eh = mtc.sporadic_task_.get();
  eh->handle_timeout(now);

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, schedule_error)
{
  OpenDDS::Test::MockLogger logger;
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, rchandle_from(&reactor_task));
  const TimeDuration one_second(1);

  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(1)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));
  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(-1));

  sporadic_task->schedule(one_second);
  reactor_task.wait_until_empty();

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, schedule_earlier)
{
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, rchandle_from(&reactor_task));
  const TimeDuration one_second(1);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(2)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value))
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .WillOnce(testing::Return(1));
  sporadic_task->schedule(two_seconds);
  reactor_task.wait_until_empty();

  EXPECT_CALL(*reactor, cancel_timer(sporadic_task->timer_id(), 0, 1))
    .WillOnce(testing::Return(1));
  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(one_second);
  reactor_task.wait_until_empty();

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, schedule_later)
{
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, rchandle_from(&reactor_task));
  const TimeDuration one_second(1);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value))
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(one_second);
  sporadic_task->schedule(two_seconds);
  reactor_task.wait_until_empty();

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, schedule_no_reactor_task)
{
  OpenDDS::Test::MockLogger logger;
  MyTimeSource time_source;
  RcHandle<MySporadicTask> sporadic_task;
  {
    MyReactor* reactor = new MyReactor;
    ThreadStatusManager tsm;
    ReactorTask_rch reactor_task = make_rch<ReactorTask>(false);
    reactor_task->open_reactor_task(&tsm, "test", reactor);
    sporadic_task = make_rch<MySporadicTask>(time_source, reactor_task);
    reactor_task->stop();
  }
  const TimeDuration two_seconds(2);

  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  sporadic_task->schedule(two_seconds);
}

TEST(dds_DCPS_SporadicTask, cancel_not_scheduled)
{
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, rchandle_from(&reactor_task));

  sporadic_task->cancel();
  reactor_task.wait_until_empty();

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, cancel_scheduled)
{
  MyTimeSource time_source;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm, "test", reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, rchandle_from(&reactor_task));
  const TimeDuration two_seconds(2);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(two_seconds);
  reactor_task.wait_until_empty();

  EXPECT_CALL(*reactor, cancel_timer(sporadic_task->timer_id(), 0, 1))
    .WillOnce(testing::Return(1));

  sporadic_task->cancel();
  reactor_task.wait_until_empty();

  reactor_task.stop();
}

TEST(dds_DCPS_SporadicTask, cancel_no_reactor_task)
{
  OpenDDS::Test::MockLogger logger;
  MyTimeSource time_source;
  RcHandle<MySporadicTask> sporadic_task;
  MyReactor* reactor = new MyReactor;
  ThreadStatusManager tsm;
  ReactorTask_rch reactor_task = make_rch<ReactorTask>(false);
  reactor_task->open_reactor_task(&tsm, "test", reactor);
  sporadic_task = make_rch<MySporadicTask>(time_source, reactor_task);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(*reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(two_seconds);
  reactor_task->wait_until_empty();

  reactor_task->stop();
  reactor_task.reset();

  sporadic_task->cancel();
}
#endif
