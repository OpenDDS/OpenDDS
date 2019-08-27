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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ReactorInterceptor::ReactorInterceptor(ACE_Reactor* reactor,
                                       ACE_thread_t owner)
  : owner_(owner)
  , condition_(mutex_)
{
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ReactorInterceptor initialized with null reactor\n"));
  }
  this->reactor(reactor);
}

ReactorInterceptor::~ReactorInterceptor()
{
}

bool ReactorInterceptor::should_execute_immediately()
{
  return ACE_OS::thr_equal(owner_, ACE_Thread::self()) ||
    reactor_is_shut_down();
}

void ReactorInterceptor::wait()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);

  if (should_execute_immediately()) {
    handle_exception_i();
    if (!reactor_is_shut_down()) {
      reactor()->purge_pending_notifications(this);
    }
  } else {
    while (!command_queue_.empty()) {
      condition_.wait();
    }
  }
}


int ReactorInterceptor::handle_exception(ACE_HANDLE /*fd*/)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->mutex_, 0);

  return handle_exception_i();
}

void ReactorInterceptor::process_command_queue_i()
{
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
  while (!command_queue_.empty()) {
    CommandPtr command = move(command_queue_.front());
    command_queue_.pop();
    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rev_guard, rev_lock)
    if (command)
      command->execute();
  }
}

int ReactorInterceptor::handle_exception_i()
{
  process_command_queue_i();
  condition_.broadcast();
  return 0;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
