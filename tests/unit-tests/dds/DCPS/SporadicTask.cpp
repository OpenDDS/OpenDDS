/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MockLogger.h"

#include <dds/DCPS/SporadicTask.h>

#include <ace/Thread_Manager.h>

#include <tests/Utils/gtestWrapper.h>

using namespace OpenDDS::DCPS;

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

  class MyReactorInterceptor : public ReactorInterceptor {
  public:
    MyReactorInterceptor(ACE_Reactor* reactor)
      : ReactorInterceptor(reactor, ACE_Thread_Manager::instance()->thr_self())
    {}

    bool reactor_is_shut_down() const { return false; }
  };

  class MySporadicTask : public SporadicTask {
  public:
    MySporadicTask(const TimeSource& time_source,
                   RcHandle<ReactorInterceptor> interceptor)
      : SporadicTask(time_source, interceptor)
    {}
    long timer_id() { return get_timer_id(); }

    MOCK_METHOD1(execute, void(const MonotonicTimePoint&));
  };

  class MyTestClass : public virtual RcObject {
  public:
    MyTestClass(const TimeSource& time_source,
                RcHandle<ReactorInterceptor> interceptor)
      : sporadic_task_(time_source, interceptor, rchandle_from(this), &MyTestClass::myfunc)
    {}

    MOCK_METHOD1(myfunc, void(const MonotonicTimePoint&));
    PmfSporadicTask<MyTestClass> sporadic_task_;
  };
}

TEST(dds_DCPS_SporadicTask, schedule)
{
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration one_second(1);
  const ACE_Time_Value now(7);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(1)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));
  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));
  EXPECT_CALL(*sporadic_task.get(), execute(MonotonicTimePoint(now)));

  sporadic_task->schedule(one_second);

  // Execute.
  RcHandle<RcEventHandler> eh = sporadic_task;
  eh->handle_timeout(now);
}

TEST(dds_DCPS_SporadicTask, schedule_pmf)
{
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  MyTestClass mtc(time_source, reactor_interceptor);
  const TimeDuration one_second(1);
  const ACE_Time_Value now(7);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(1)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));
  EXPECT_CALL(reactor, schedule_timer(&mtc.sporadic_task_, 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));
  EXPECT_CALL(mtc, myfunc(MonotonicTimePoint(now)));

  mtc.sporadic_task_.schedule(one_second);

  // Execute.
  RcEventHandler* eh = &mtc.sporadic_task_;
  eh->handle_timeout(now);
}

TEST(dds_DCPS_SporadicTask, schedule_error)
{
  OpenDDS::Test::MockLogger logger;
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration one_second(1);

  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(1)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));
  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(-1));

  sporadic_task->schedule(one_second);
}

TEST(dds_DCPS_SporadicTask, schedule_earlier)
{
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration one_second(1);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .Times(2)
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value))
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .WillOnce(testing::Return(1));
  sporadic_task->schedule(two_seconds);

  EXPECT_CALL(reactor, cancel_timer(sporadic_task->timer_id(), 0, 1))
    .WillOnce(testing::Return(1));
  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, one_second.value(), ACE_Time_Value::zero))
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(one_second);
}

TEST(dds_DCPS_SporadicTask, schedule_later)
{
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value))
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(two_seconds);
  sporadic_task->schedule(two_seconds);
}

TEST(dds_DCPS_SporadicTask, schedule_no_interceptor)
{
  OpenDDS::Test::MockLogger logger;
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  reactor_interceptor.reset();
  sporadic_task->schedule(two_seconds);
}

TEST(dds_DCPS_SporadicTask, cancel_not_scheduled)
{
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);

  sporadic_task->cancel();
}

TEST(dds_DCPS_SporadicTask, cancel_scheduled)
{
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(two_seconds);

  EXPECT_CALL(reactor, cancel_timer(sporadic_task->timer_id(), 0, 1))
    .WillOnce(testing::Return(1));

  sporadic_task->cancel();
}

TEST(dds_DCPS_SporadicTask, cancel_no_interceptor)
{
  OpenDDS::Test::MockLogger logger;
  MyTimeSource time_source;
  MyReactor reactor;
  RcHandle<MyReactorInterceptor> reactor_interceptor = make_rch<MyReactorInterceptor>(&reactor);
  RcHandle<MySporadicTask> sporadic_task = make_rch<MySporadicTask>(time_source, reactor_interceptor);
  const TimeDuration two_seconds(2);

  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  EXPECT_CALL(time_source, monotonic_time_point_now())
    .WillOnce(testing::Return(MonotonicTimePoint::zero_value));

  EXPECT_CALL(reactor, schedule_timer(sporadic_task.get(), 0, two_seconds.value(), ACE_Time_Value::zero))
    .Times(1)
    .WillOnce(testing::Return(1));

  sporadic_task->schedule(two_seconds);
  reactor_interceptor.reset();
  sporadic_task->cancel();
}
