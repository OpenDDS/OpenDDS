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
  , on_queue_(false)
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

bool ReactorInterceptor::should_execute_immediately()
{
  return ACE_OS::thr_equal(owner_, ACE_Thread::self()) ||
    reactor_is_shut_down();
}

int ReactorInterceptor::handle_exception(ACE_HANDLE /*fd*/)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  process_command_queue_i();
  return 0;
}

void ReactorInterceptor::process_command_queue_i()
{
  Queue cq;
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  state_ = PROCESSING;
  if (!command_queue_.empty()) {
    cq.swap(command_queue_);
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rev_guard(rev_lock);
    for (Queue::const_iterator pos = cq.begin(), limit = cq.end(); pos != limit; ++pos) {
      (*pos)->dequeue();
      (*pos)->execute();
      (*pos)->executed();
    }
  }
  if (!command_queue_.empty()) {
    state_ = NOTIFIED;
    reactor()->notify(this);
  } else {
    state_ = NONE;
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
