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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReactorInterceptor : public RcEventHandler {
public:

  class Command
  : public PoolAllocationBase
  , public EnableContainerSupportedUniquePtr<Command> {
  public:
    virtual ~Command() { }
    virtual void execute() = 0;
  };
  typedef container_supported_unique_ptr<Command> CommandPtr;

  bool should_execute_immediately();

  template<typename T>
  void execute_or_enqueue(T& t)
  {
    if (should_execute_immediately()) {
      {
        ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);
        process_command_queue_i();
      }
      t.execute();
    } else {
      enqueue(t);
    }
  }

  template<typename T>
  void enqueue(T& t)
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);
    command_queue_.push(CommandPtr(new T(t)));
    this->reactor()->notify(this);
  }

  void wait();

  virtual bool reactor_is_shut_down() const = 0;

protected:
  ReactorInterceptor(ACE_Reactor* reactor,
                     ACE_thread_t owner);

  virtual ~ReactorInterceptor();
  int handle_exception(ACE_HANDLE /*fd*/);
  int handle_exception_i();
  void process_command_queue_i();

  ACE_thread_t owner_;
  ACE_Thread_Mutex mutex_;
  ACE_Condition_Thread_Mutex condition_;
  OPENDDS_QUEUE(CommandPtr) command_queue_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_REACTORINTERCEPTOR_H  */
