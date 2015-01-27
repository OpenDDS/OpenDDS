/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REACTORINTERCEPTOR_H
#define OPENDDS_DCPS_REACTORINTERCEPTOR_H

#include <queue>
#include "ace/Reactor.h"
#include "ace/Thread.h"
#include "ace/Condition_T.h"

namespace OpenDDS {
namespace DCPS {

class ReactorInterceptor : public ACE_Event_Handler {
public:

  class Command {
  public:
    virtual ~Command() { }
    virtual void execute() = 0;
  };

  ReactorInterceptor(ACE_Reactor* reactor,
                     ACE_thread_t owner);
  ~ReactorInterceptor();

  template<typename T>
  void execute_or_enqueue(T& t)
  {
    if (owner_ == ACE_Thread::self()) {
      t.execute();
    } else {
      ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);
      command_queue_.push(new T(t));
      this->reactor()->notify(this);
    }
  }

  void wait();
private:
  int handle_exception(ACE_HANDLE /*fd*/);
  ACE_thread_t owner_;
  ACE_Thread_Mutex mutex_;
  ACE_Condition<ACE_Thread_Mutex> condition_;
  std::queue<Command*> command_queue_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_REACTORINTERCEPTOR_H  */
