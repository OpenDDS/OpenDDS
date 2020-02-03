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
{
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

int ReactorInterceptor::handle_exception(ACE_HANDLE /*fd*/)
{
  process_command_queue_i();
  return 0;
}

void ReactorInterceptor::process_command_queue_i()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
  while (!command_queue_.empty()) {
    CommandPtr command = command_queue_.front();
    command_queue_.pop();
    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rev_guard, rev_lock);
    command->execute();
    command->executed();
    command.reset();
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
