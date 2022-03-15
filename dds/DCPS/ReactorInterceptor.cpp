/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ace/Log_Msg.h"
#include "ace/Reverse_Lock_T.h"
#include "ace/Synch.h"

#include "ReactorInterceptor.h"
#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ReactorInterceptor::Command::Command()
  : executed_(false)
  , condition_(mutex_)
  , reactor_(0)
{}

void ReactorInterceptor::Command::wait() const
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  while (!executed_) {
    condition_.wait(thread_status_manager);
  }
}

ReactorInterceptor::ReactorInterceptor(ACE_Reactor* reactor,
                                       ACE_thread_t owner)
  : owner_(owner)
  , state_(NONE)
{
  RcEventHandler::reactor(reactor);
}

ReactorInterceptor::~ReactorInterceptor()
{
}

ReactorInterceptor::CommandPtr ReactorInterceptor::execute_or_enqueue(CommandPtr command)
{
  OPENDDS_ASSERT(command);

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  // Only allow immediate execution if running on the reactor thread, otherwise we risk deadlock
  // when calling into the reactor object.
  const bool is_owner = ACE_OS::thr_equal(owner_, ACE_Thread::self());

  // If state is set to processing, the conents of command_queue_ have been swapped out
  // so immediate execution may run jobs out of the expected order.
  const bool is_not_processing = state_ != PROCESSING;

  // If the command_queue_ is not empty, allowing execution will potentially run unexpected code
  // which is problematic since we may be holding locks used by the unexpected code.
  const bool is_empty = command_queue_.empty();

  // If all three of these conditions are met, it should be safe to execute
  const bool is_safe_to_execute = is_owner && is_not_processing && is_empty;

  // Even if it's not normally safe to execute, allow immediate execution if the reactor is shut down
  const bool immediate = is_safe_to_execute || reactor_is_shut_down();

  // Always set reactor and push to the queue
  ACE_Reactor* local_reactor = ACE_Event_Handler::reactor();
  command->set_reactor(local_reactor);
  command_queue_.push_back(command);

  // But depending on whether we're running it immediately or not, we either process or notify
  if (immediate) {
    process_command_queue_i(guard);
  } else if (state_ == NONE) {
    state_ = NOTIFIED;
    guard.release();
    local_reactor->notify(this);
  }
  return command;
}

int ReactorInterceptor::handle_exception(ACE_HANDLE /*fd*/)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  process_command_queue_i(guard);
  return 0;
}

void ReactorInterceptor::process_command_queue_i(ACE_Guard<ACE_Thread_Mutex>& guard)
{
  Queue cq;
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);

  state_ = PROCESSING;
  if (!command_queue_.empty()) {
    cq.swap(command_queue_);
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rev_guard(rev_lock);
    for (Queue::const_iterator pos = cq.begin(), limit = cq.end(); pos != limit; ++pos) {
      (*pos)->execute();
      (*pos)->executed();
    }
  }
  if (!command_queue_.empty()) {
    state_ = NOTIFIED;
    ACE_Reactor* const reactor = ACE_Event_Handler::reactor();
    guard.release();
    reactor->notify(this);
  } else {
    state_ = NONE;
  }
}

void ReactorInterceptor::reactor(ACE_Reactor *reactor)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  ACE_Event_Handler::reactor(reactor);
}

ACE_Reactor* ReactorInterceptor::reactor() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return ACE_Event_Handler::reactor();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
