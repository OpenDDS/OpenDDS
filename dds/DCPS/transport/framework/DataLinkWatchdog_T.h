/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_DATALINKWATCHDOG_T_H
#define DCPS_DATALINKWATCHDOG_T_H

#include "ace/Global_Macros.h"
#include "ace/Event_Handler.h"
#include "ace/Log_Msg.h"
#include "ace/Mutex.h"
#include "ace/Reactor.h"
#include "ace/Time_Value.h"
#include "ace/OS_NS_time.h"
#include "ace/Reverse_Lock_T.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "dds/DCPS/Time_Helper.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataLinkWatchdog : public ReactorInterceptor {
public:

  bool schedule(const void* arg = 0) {
    execute_or_enqueue(new ScheduleCommand (this, arg, false));
    return true;
  }

  bool schedule_now(const void* arg = 0) {
    execute_or_enqueue(new ScheduleCommand(this, arg, true));
    return true;
  }

  ReactorInterceptor::CommandPtr cancel() {
    return execute_or_enqueue(new CancelCommand(this));
  }

  int handle_timeout(const ACE_Time_Value& now_time_value, const void* arg) {
    const MonotonicTimePoint now(now_time_value);
    TimeDuration timeout_in = next_timeout();

    if (!timeout_in.is_zero()) {
      const MonotonicTimePoint timeout_at(epoch_ + timeout_in);
      if (now > timeout_at) {
        on_timeout(arg);
        {
          cancel_i();
        }
        return 0;
      }
    }

    on_interval(arg);

    {
      if (!schedule_i(arg, false)) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("DataLinkWatchdog::handle_timeout: ")
                   ACE_TEXT("unable to reschedule watchdog timer!\n")));
      }
    }

    return 0;
  }

protected:
  DataLinkWatchdog(ACE_Reactor* reactor,
                   ACE_thread_t owner)
    : ReactorInterceptor(reactor, owner)
    , timer_id_(-1)
    , cancelled_(false)
  {}

  virtual ~DataLinkWatchdog() {
  }

  virtual TimeDuration next_interval() = 0;
  virtual void on_interval(const void* arg) = 0;

  virtual TimeDuration next_timeout() { return TimeDuration::zero_value; }
  virtual void on_timeout(const void* /*arg*/) {}

private:
  class CommandBase : public Command {
  public:
    CommandBase(DataLinkWatchdog* watchdog)
      : watchdog_(watchdog)
    { }
  protected:
    DataLinkWatchdog* watchdog_;
  };

  class ScheduleCommand : public CommandBase {
  public:
    ScheduleCommand (DataLinkWatchdog* watchdog, const void* arg, bool nodelay)
      : CommandBase(watchdog)
      , arg_ (arg)
      , nodelay_ (nodelay)
    { }

    virtual void execute() {
      watchdog_->schedule_i(arg_, nodelay_);
    }

  private:
    const void* arg_;
    bool nodelay_;
  };

  class CancelCommand : public CommandBase {
  public:
    CancelCommand(DataLinkWatchdog* watchdog)
      : CommandBase(watchdog)
    { }

    virtual void execute() {
      watchdog_->cancel_i();
    }
  };

  long timer_id_;

  MonotonicTimePoint epoch_;
  bool cancelled_;

  bool schedule_i(const void* arg, bool nodelay) {
    if (this->cancelled_) return true;

    TimeDuration delay;
    if (!nodelay) {
      delay = next_interval();
    }

    if (epoch_.is_zero()) {
      epoch_.set_to_now();
    }

    long timer_id = -1;
    {
      timer_id = reactor()->schedule_timer(this,  // event_handler
                                           arg,
                                           delay.value());

      if (timer_id == -1) {
        ACE_ERROR_RETURN ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("DataLinkWatchdog::schedule_i: ")
                  ACE_TEXT("failed to register timer %p!\n"),
                  ACE_TEXT("schedule_timer")), false);
      }
    }

    //after re-acquiring lock_ need to check cancelled_
    if (this->cancelled_) {
      reactor()->cancel_timer(timer_id);
      return true;
    } else {
      this->timer_id_ = timer_id;
    }

    return this->timer_id_ != -1;
  }

  void cancel_i() {
    if (this->timer_id_ == -1) return;

    this->timer_id_ = -1;
    this->cancelled_ = true;
    reactor()->cancel_timer(this);
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_DATALINKWATCHDOG_T_H */
