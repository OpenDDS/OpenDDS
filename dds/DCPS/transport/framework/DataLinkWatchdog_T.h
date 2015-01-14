/*
 * $Id$
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
#include <queue>

namespace OpenDDS {
namespace DCPS {

template<typename ACE_LOCK>
class DataLinkWatchdog : public ACE_Event_Handler {
public:
  typedef DataLinkWatchdog<ACE_LOCK> ThisType;

  virtual ~DataLinkWatchdog() {
    ACE_GUARD(ACE_LOCK, guard, this->lock_);
    if(this->reactor_) {
      this->reactor_->purge_pending_notifications(this);
    }
    dump_command_queue();
    cancel_i();
  }

  bool schedule(ACE_Reactor* reactor, const void* arg = 0) {
    ACE_GUARD_RETURN(ACE_LOCK, guard, this->lock_, false);
    command_queue_.push(new ScheduleCommand(reactor, arg, false));
    reactor->notify(this);
    return true;
  }

  bool schedule_now(ACE_Reactor* reactor, const void* arg = 0) {
    ACE_GUARD_RETURN(ACE_LOCK, guard, this->lock_, false);
    command_queue_.push(new ScheduleCommand (reactor, arg, true));
    reactor->notify(this);
    return true;
  }

  void cancel() {
    ACE_GUARD(ACE_LOCK, guard, this->lock_);
    if (reactor_) {
      dump_command_queue();
      command_queue_.push(new CancelCommand ());
      reactor_->notify(this);
    }
    else {
      cancel_i();
    }
  }

  int handle_timeout(const ACE_Time_Value& now, const void* arg) {
    ACE_Time_Value timeout = next_timeout();

    if (timeout != ACE_Time_Value::zero) {
      timeout += this->epoch_;
      if (now > timeout) {
        on_timeout(arg);
        {
          ACE_GUARD_RETURN(ACE_LOCK, guard, this->lock_, 0);
          cancel_i();
        }
        return 0;
      }
    }

    on_interval(arg);

    {
      ACE_GUARD_RETURN(ACE_LOCK, guard, this->lock_, 0);
      if (!schedule_i(reactor(), arg, false)) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("DataLinkWatchdog::handle_timeout: ")
                   ACE_TEXT("unable to reschedule watchdog timer!\n")));
      }
    }

    return 0;
  }

  int handle_exception(ACE_HANDLE /*fd*/) {
    ACE_GUARD_RETURN(ACE_LOCK, guard, this->lock_, 0);

    while (!command_queue_.empty()) {
      Command* command = command_queue_.front();
      command_queue_.pop();
      command->execute(this);
      delete command;
    }
    return 0;
  }

protected:
  DataLinkWatchdog()
    : reverse_lock_(lock_),
      reactor_(0),
      timer_id_(-1),
      cancelled_(false)
  {}

  virtual ACE_Time_Value next_interval() = 0;
  virtual void on_interval(const void* arg) = 0;

  virtual ACE_Time_Value next_timeout() { return ACE_Time_Value::zero; }
  virtual void on_timeout(const void* /*arg*/) {}

private:
  class Command {
  public:
    virtual ~Command() { }
    virtual void execute(ThisType* watchdog) = 0;
  };

  class ScheduleCommand : public Command {
  public:
    ScheduleCommand (ACE_Reactor* reactor, const void* arg, bool nodelay) :
    reactor_ (reactor),
      arg_ (arg),
      nodelay_ (nodelay)
      { }

    virtual void execute(ThisType* watchdog) {
      watchdog->schedule_i(reactor_, arg_, nodelay_);
    }

  private:
    ACE_Reactor* reactor_;
    const void* arg_;
    bool nodelay_;
  };

  class CancelCommand : public Command {
  public:
    virtual void execute(ThisType* watchdog) {
      watchdog->cancel_i();
    }
  };

  ACE_LOCK lock_;
  typedef ACE_Reverse_Lock<ACE_LOCK> Reverse_Lock_t;
  Reverse_Lock_t reverse_lock_;

  ACE_Reactor* reactor_;
  long timer_id_;

  ACE_Time_Value epoch_;
  bool cancelled_;

  std::queue<Command*> command_queue_;

  bool schedule_i(ACE_Reactor* reactor, const void* arg, bool nodelay) {
    if (this->cancelled_) return true;

    ACE_Time_Value delay;
    if (!nodelay) delay = next_interval();

    if (this->epoch_ == ACE_Time_Value::zero) {
      this->epoch_ = ACE_OS::gettimeofday();
    }

    long timer_id = -1;
    {
      ACE_GUARD_RETURN(Reverse_Lock_t, unlock_guard, reverse_lock_, false);
      timer_id = reactor->schedule_timer(this,  // event_handler
                                         arg,
                                         delay);

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
      reactor->cancel_timer(timer_id);
      return true;
    }
    else {
      this->timer_id_ = timer_id;
      this->reactor_ = reactor;
    }

    return this->timer_id_ != -1;
  }

  void cancel_i() {
    if (this->timer_id_ == -1) return;

    ACE_Reactor* reactor = this->reactor_;
    this->timer_id_ = -1;
    this->reactor_ = 0;
    this->cancelled_ = true;

    {
      ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_lock_);
      reactor->cancel_timer(this);
    }
  }

  void dump_command_queue() {
    while (!command_queue_.empty ()) {
      delete command_queue_.front ();
      command_queue_.pop ();
    }
  }
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_DATALINKWATCHDOG_T_H */
