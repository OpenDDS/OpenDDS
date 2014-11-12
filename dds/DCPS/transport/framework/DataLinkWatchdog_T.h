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
#include "dds/DCPS/async_debug.h"

namespace OpenDDS {
namespace DCPS {

template<typename ACE_LOCK>
class DataLinkWatchdog : public ACE_Event_Handler {
public:
  virtual ~DataLinkWatchdog() {
    cancel();
  }

  bool schedule(ACE_Reactor* reactor, const void* arg = 0) {
    ACE_GUARD_RETURN(ACE_LOCK,
                     guard,
                     this->lock_,
                     false);

    return schedule_i(reactor, arg, false);
  }

  bool schedule_now(ACE_Reactor* reactor, const void* arg = 0) {
    ACE_GUARD_RETURN(ACE_LOCK,
                     guard,
                     this->lock_,
                     false);

    return schedule_i(reactor, arg, true);
  }

  void cancel() {
    ACE_GUARD(ACE_LOCK,
              guard,
              this->lock_);

    if (this->timer_id_ == -1) return;

    long timer_id = this->timer_id_;
    ACE_Reactor* reactor = this->reactor_;
    this->timer_id_ = -1;
    this->reactor_ = 0;
    this->cancelled_ = true;
    int n_cancelled;

    {
      ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_lock_);
      n_cancelled = reactor->cancel_timer(this);
    }
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataLinkWatchdog_T.h::cancel --> after cancel_timer timer_id: %d n: %d %@\n", timer_id, n_cancelled, static_cast<ACE_Event_Handler*>(this)));
  }

  int handle_timeout(const ACE_Time_Value& now, const void* arg) {
    ACE_Time_Value timeout = next_timeout();

    if (timeout != ACE_Time_Value::zero) {
      timeout += this->epoch_;
      if (now > timeout) {
        on_timeout(arg);
        cancel();
        return 0;
      }
    }

    on_interval(arg);

    if (!schedule(reactor(), arg)) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("DataLinkWatchdog::handle_timeout: ")
                 ACE_TEXT("unable to reschedule watchdog timer!\n")));
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
  ACE_LOCK lock_;
  typedef ACE_Reverse_Lock<ACE_LOCK> Reverse_Lock_t;
  Reverse_Lock_t reverse_lock_;

  ACE_Reactor* reactor_;
  long timer_id_;

  ACE_Time_Value epoch_;
  bool cancelled_;

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
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataLinkWatchdog_T.h::schedule_i --> schedule_timer: %d (%@)\n", timer_id, static_cast<ACE_Event_Handler*>(this)));

      if (timer_id == -1) {
        ACE_ERROR_RETURN ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("DataLinkWatchdog::schedule_i: ")
                  ACE_TEXT("failed to register timer %p!\n"),
                  ACE_TEXT("schedule_timer")), false);
      }
    }

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataLinkWatchdog_T.h::schedule_i --> after schedule timer this->timer_id_: %d and timer_id: %d %@\n", this->timer_id_, timer_id,
      static_cast<ACE_Event_Handler*>(this)));

    //after re-acquiring lock_ need to check cancelled_
    //### Thought maybe would have to check timer_id_ for cancellation/prior completion
    //### this check would from scheduling multiple timers for the watchdog at the same time while starting
    //### but seems that if you check that value you may lose necessary timers down the line so simply allow
    //### multiple timers to be scheduled, since  they are not interval timers it won't keep firing after each expires the first time
    if (this->cancelled_) {
       //### Debug statements to track where associate is failing
       if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataLinkWatchdog_T.h::schedule_i --> about to cancel timer timer_id: %d watchdog:(%@)\n", timer_id, static_cast<ACE_Event_Handler*>(this)));
      reactor->cancel_timer(timer_id);
      return true;
    }
    else {
      this->timer_id_ = timer_id;
      this->reactor_ = reactor;
    }

    return this->timer_id_ != -1;
  }
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_DATALINKWATCHDOG_T_H */
