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
#include "ace/Condition_Thread_Mutex.h"
#include "dcps_export.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReactorInterceptor : public ACE_Event_Handler {
public:

  class Command {
  public:
    virtual ~Command() { }
    virtual void execute() = 0;
  };

  ReactorInterceptor(ACE_Reactor* reactor,
                     ACE_thread_t owner);

  bool should_execute_immediately();

  template<typename T>
  void execute_or_enqueue(T& t)
  {
    if (should_execute_immediately()) {
      t.execute();
    } else {
      enqueue(t);
    }
  }

  template<typename T>
  void enqueue(T& t)
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);
    command_queue_.push(new T(t));
    this->reactor()->notify(this);
    ++registration_counter_;
  }

  void wait();

  void destroy();

  virtual bool reactor_is_shut_down() const = 0;

protected:
  virtual ~ReactorInterceptor();

private:
  int handle_exception(ACE_HANDLE /*fd*/);
  int handle_exception_i(ACE_HANDLE /*fd*/, ACE_Guard<ACE_Thread_Mutex>& guard);
  ACE_thread_t owner_;
  ACE_Thread_Mutex mutex_;
  ACE_Condition_Thread_Mutex condition_;
  std::queue<Command*> command_queue_;
  ACE_UINT64 registration_counter_;
  bool destroy_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_REACTORINTERCEPTOR_H  */
