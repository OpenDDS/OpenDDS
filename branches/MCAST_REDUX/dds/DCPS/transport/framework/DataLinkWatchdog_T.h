/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
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
#include "ace/Time_Value.h"

#include "ace/OS_NS_time.h"

namespace OpenDDS {
namespace DCPS {

template<typename LINK>
class DataLinkWatchdog : public ACE_Event_Handler {
public:
  typedef LINK* ptr_type;

  explicit DataLinkWatchdog(ptr_type link)
    : link_(link),
      reactor_(0),
      timer_id_(-1)
  {}

  virtual ~DataLinkWatchdog() {
    cancel();
  }
  
  bool schedule(ACE_Reactor* reactor, const void* arg = 0) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex,
                     guard,
                     this->lock_,
                     false);

    if (this->timer_id_ != -1) return false;

    this->epoch_ = ACE_OS::gettimeofday();
    
    this->reactor_ = reactor;
    this->timer_id_ =
      reactor->schedule_timer(this,  // event_handler
                              arg,
                              ACE_Time_Value::zero,
                              next_interval());
   
    return this->timer_id_ != -1; 
  }

  void cancel() {
    ACE_GUARD(ACE_Thread_Mutex,
              guard,
              this->lock_);

    if (this->timer_id_ != -1) {
      this->reactor_->cancel_timer(this->timer_id_);

      this->reactor_ = 0;
      this->timer_id_ = -1;
    }
  }
  
  int handle_timeout(const ACE_Time_Value& now, const void* arg) {
    ACE_Time_Value deadline(this->epoch_);
    deadline += next_timeout();

    if (now > deadline) {
      on_timeout(arg);
      cancel();
      return 0;
    }

    if (!on_interval(arg)) {
      cancel();
      return 0;
    }
  
    if (this->reactor_->reset_timer_interval(this->timer_id_,
                                             next_interval()) != 0) { 
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("DataLinkWatchdog::handle_timeout: ")
                 ACE_TEXT("unable to reset timer interval!\n")));
    }
    
    return 0;
  }

protected:
  ptr_type link_;
  
  virtual ACE_Time_Value next_interval() = 0;
  virtual bool on_interval(const void* arg) = 0;

  virtual ACE_Time_Value next_timeout() = 0;
  virtual void on_timeout(const void* arg) = 0;

private:
  ACE_Thread_Mutex lock_;

  ACE_Reactor* reactor_;
  long timer_id_;

  ACE_Time_Value epoch_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_DATALINKWATCHDOG_T_H */
