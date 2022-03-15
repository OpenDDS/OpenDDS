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
#include "RcEventHandler.h"
#include "dcps_export.h"
#include "unique_ptr.h"
#include "RcHandle_T.h"
#include "ConditionVariable.h"

#include <ace/Reactor.h>
#include <ace/Thread.h>
#include <ace/Thread_Mutex.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReactorInterceptor : public RcEventHandler {
public:

  class OpenDDS_Dcps_Export Command
  : public RcObject {
  public:
    Command();
    virtual ~Command() { }

    virtual void execute() = 0;

    void executed()
    {
      ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
      executed_ = true;
      condition_.notify_all();
    }

    void wait() const;

  protected:
    const ACE_Reactor* reactor() const { return reactor_; }
    ACE_Reactor* reactor() { return reactor_; }

  private:
    friend class OpenDDS::DCPS::ReactorInterceptor;
    void set_reactor(ACE_Reactor* reactor) { reactor_ = reactor; }

    bool executed_;
    mutable ACE_Thread_Mutex mutex_;
    mutable ConditionVariable<ACE_Thread_Mutex> condition_;
    ACE_Reactor* reactor_;
  };
  typedef RcHandle<Command> CommandPtr;

  CommandPtr execute_or_enqueue(CommandPtr command);

  virtual bool reactor_is_shut_down() const = 0;

  virtual void reactor(ACE_Reactor *reactor);
  virtual ACE_Reactor* reactor() const;

protected:

  enum ReactorState {
    NONE,
    NOTIFIED,
    PROCESSING
  };

  ReactorInterceptor(ACE_Reactor* reactor,
                     ACE_thread_t owner);

  virtual ~ReactorInterceptor();
  int handle_exception(ACE_HANDLE /*fd*/);
  void process_command_queue_i(ACE_Guard<ACE_Thread_Mutex>& guard);

  ACE_thread_t owner_;
  mutable ACE_Thread_Mutex mutex_;
  typedef OPENDDS_VECTOR(CommandPtr) Queue;
  Queue command_queue_;
  ReactorState state_;
};

typedef RcHandle<ReactorInterceptor> ReactorInterceptor_rch;
typedef WeakRcHandle<ReactorInterceptor> ReactorInterceptor_wrch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_REACTORINTERCEPTOR_H  */
