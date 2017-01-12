/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Watchdog.h"
#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {
  struct CommandBase : ReactorInterceptor::Command {
    explicit CommandBase(ReactorInterceptor* inter)
      : interceptor_(inter) {}
    ReactorInterceptor* const interceptor_;
  };

  struct ScheduleCommand : CommandBase {
    ScheduleCommand(ReactorInterceptor* inter,
                    const void* act,
                    const ACE_Time_Value& delay,
                    const ACE_Time_Value& interval,
                    long* timer_id)
    : CommandBase(inter)
    , act_(act)
    , delay_(delay)
    , interval_(interval)
    , timer_id_(timer_id)
    {}

    void execute()
    {
      *timer_id_ = interceptor_->reactor()->schedule_timer(interceptor_, act_,
                                                           delay_, interval_);
    }

    const void* const act_;
    const ACE_Time_Value delay_;
    const ACE_Time_Value interval_;
    long* timer_id_;
  };

  struct CancelCommand : CommandBase {
    CancelCommand(ReactorInterceptor* inter, long timer_id)
    : CommandBase(inter)
    , timer_id_(timer_id)
    {}

    void execute()
    {
      if (timer_id_ >= 0) {
        interceptor_->reactor()->cancel_timer(timer_id_);
      } else {
        interceptor_->reactor()->cancel_timer(interceptor_);
      }
    }

    const long timer_id_;
  };

  struct ResetCommand : CommandBase {
    ResetCommand(ReactorInterceptor* inter, long timer_id,
                 const ACE_Time_Value& interval)
    : CommandBase(inter)
    , timer_id_(timer_id)
    , interval_(interval)
    {}

    void execute()
    {
      interceptor_->reactor()->reset_timer_interval(timer_id_, interval_);
    }

    const long timer_id_;
    const ACE_Time_Value interval_;
  };
}

Watchdog::Watchdog(const ACE_Time_Value& interval)
  : ReactorInterceptor(TheServiceParticipant->reactor(),
                       TheServiceParticipant->reactor_owner())
  , interval_(interval)
{
}

Watchdog::~Watchdog()
{
}

bool Watchdog::reactor_is_shut_down() const
{
  return TheServiceParticipant->is_shut_down();
}

void Watchdog::reset_interval(const ACE_Time_Value& interval)
{
  if (this->interval_ != interval) {
    this->interval_ = interval;
    this->reschedule_deadline();
  }
}

long Watchdog::schedule_timer(const void* act, const ACE_Time_Value& interval)
{
  return schedule_timer(act, interval, interval);
}

long Watchdog::schedule_timer(const void* act, const ACE_Time_Value& delay, const ACE_Time_Value& interval)
{
  long timer_id = -1;
  ScheduleCommand c(this, act, delay, interval, &timer_id);
  execute_or_enqueue(c);
  wait();
  return timer_id;
}

int Watchdog::cancel_timer(long timer_id)
{
  CancelCommand c(this, timer_id);
  execute_or_enqueue(c);
  return 1;
}

void Watchdog::cancel_all()
{
  CancelCommand c(this, -1);
  execute_or_enqueue(c);
}

int Watchdog::reset_timer_interval(long timer_id)
{
  ResetCommand c(this, timer_id, interval_);
  execute_or_enqueue(c);
  return 0;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
