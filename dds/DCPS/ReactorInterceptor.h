/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REACTORINTERCEPTOR_H
#define OPENDDS_DCPS_REACTORINTERCEPTOR_H

#include "PoolAllocator.h"
#include "PoolAllocationBase.h"
#include "ace/Reactor.h"
#include "ace/Thread.h"
#include "ace/Condition_Thread_Mutex.h"
#include "RcEventHandler.h"
#include "dcps_export.h"
#include "unique_ptr.h"
#include "RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReactorInterceptor : public RcEventHandler {
public:

  class Command
  : public RcObject {
  public:
    Command() : executed_(false), condition_(mutex_) {}
    virtual ~Command() { }
    virtual void execute() = 0;

    void reset() {
      ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
      executed_ = false;
    }

    void wait() {
      ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
      while (!executed_) {
        condition_.wait();
      }
    }

    void signal() {
      ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
      executed_ = true;
      condition_.signal();
    }

  private:
    bool executed_;
    ACE_Thread_Mutex mutex_;
    ACE_Condition_Thread_Mutex condition_;
  };
  typedef RcHandle<Command> CommandPtr;

  bool should_execute_immediately();

  CommandPtr execute_or_enqueue(Command* c)
  {
    const bool immediate = should_execute_immediately();
    CommandPtr command = enqueue(c, immediate);
    if (should_execute_immediately()) {
      process_command_queue_i();
    }
    return command;
  }

  CommandPtr enqueue(Command* c, bool immediate = false)
  {
    c->reset();
    CommandPtr command = rchandle_from(c);
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, CommandPtr());
    command_queue_.push(command);
    if (!immediate) {
      reactor()->notify(this);
    }
    return command;
  }

  virtual bool reactor_is_shut_down() const = 0;

protected:
  ReactorInterceptor(ACE_Reactor* reactor,
                     ACE_thread_t owner);

  virtual ~ReactorInterceptor();
  int handle_exception(ACE_HANDLE /*fd*/);
  void process_command_queue_i();

  ACE_thread_t owner_;
  ACE_Thread_Mutex mutex_;
  OPENDDS_QUEUE(CommandPtr) command_queue_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_REACTORINTERCEPTOR_H  */
