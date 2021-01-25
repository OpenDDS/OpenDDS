/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "ace/ACE.h"

ACE_INLINE
OpenDDS::DCPS::ThreadSynchResource::ThreadSynchResource()
  : handle_(ACE_INVALID_HANDLE)
  , timeout_(0)
{
  DBG_ENTRY_LVL("ThreadSynchResource","ThreadSynchResource",6);
}

ACE_INLINE int
OpenDDS::DCPS::ThreadSynchResource::wait_to_unclog()
{
  DBG_ENTRY_LVL("ThreadSynchResource","wait_to_unclog",6);

  if (ACE::handle_write_ready(handle_, timeout_ ? &timeout_->value() : 0) == -1) {
    if (errno == ETIME) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: handle_write_ready timed out\n"));
      this->notify_lost_on_backpressure_timeout();

    } else {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: ACE::handle_write_ready return -1 while waiting "
                 " to unclog. %p\n", ACE_TEXT("handle_write_ready")));
    }

    return -1;
  }

  return 0;
}

ACE_INLINE void
OpenDDS::DCPS::ThreadSynchResource::set_handle(ACE_HANDLE handle)
{
  handle_ = handle;
}
