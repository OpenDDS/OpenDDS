/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReactorInterceptor.h"

#include "ace/Log_Msg.h"

namespace OpenDDS {
namespace DCPS {

ReactorInterceptor::ReactorInterceptor(ACE_Reactor* reactor,
                                       ACE_thread_t owner)
  : owner_(owner)
  , condition_(mutex_)
  , registration_counter_(0)
  , destroy_(false)
{
  if (reactor == 0) {
    ACE_DEBUG((LM_ERROR, "(%P|%t) ERROR: ReactorInterceptor initialized with null reactor\n"));
  }
  this->reactor(reactor);
}

ReactorInterceptor::~ReactorInterceptor()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);

  // Dump the command queue.
  while (!command_queue_.empty ()) {
    delete command_queue_.front ();
    command_queue_.pop ();
  }
}

bool ReactorInterceptor::should_execute_immediately()
{
  return owner_ == ACE_Thread::self() ||
    reactor_is_shut_down();
}

void ReactorInterceptor::wait()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);

  if (should_execute_immediately()) {
    ++registration_counter_;
    handle_exception_i(ACE_INVALID_HANDLE, guard);
  } else {
    while (!command_queue_.empty()) {
      condition_.wait();
    }
  }
}

void ReactorInterceptor::destroy()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, this->mutex_);
  if (!reactor_is_shut_down() && registration_counter_ > 0) {
    // Wait until we get handle exception.
    destroy_ = true;
  } else {
    guard.release();
    delete this;
  }
}

int ReactorInterceptor::handle_exception(ACE_HANDLE /*fd*/)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->mutex_, 0);

  return handle_exception_i(ACE_INVALID_HANDLE, guard);
}

int ReactorInterceptor::handle_exception_i(ACE_HANDLE /*fd*/, ACE_Guard<ACE_Thread_Mutex>& guard)
{
  --registration_counter_;

  if (registration_counter_ == 0 && destroy_) {
    guard.release();
    delete this;
    return 0;
  }

  while (!command_queue_.empty()) {
    Command* command = command_queue_.front();
    command_queue_.pop();
    command->execute();
    delete command;
  }

  condition_.signal();

  return 0;
}

}

}
