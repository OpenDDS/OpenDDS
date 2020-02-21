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
  , state_(NONE)
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
  OPENDDS_DEQUE(CommandPtr) cq;
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  state_ = PROCESSING;
  while (!command_queue_.empty()) {
    cq.swap(command_queue_);
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rev_guard(rev_lock);
    while (!cq.empty()) {
      CommandPtr command = cq.front();
      cq.pop_front();
      command->execute();
      command->executed();
    }
  }
  state_ = NONE;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
