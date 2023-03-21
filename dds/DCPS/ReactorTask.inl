/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
ACE_Reactor* ReactorTask::get_reactor()
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  return reactor_;
}

ACE_INLINE
const ACE_Reactor* ReactorTask::get_reactor() const
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  wait_for_startup_i();
  return reactor_;
}

ACE_INLINE
ACE_thread_t ReactorTask::get_reactor_owner() const
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  wait_for_startup_i();
  return reactor_owner_;
}

ACE_INLINE
ACE_Proactor* ReactorTask::get_proactor()
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  return proactor_;
}

ACE_INLINE
const ACE_Proactor* ReactorTask::get_proactor() const
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  return proactor_;
}

ACE_INLINE
void ReactorTask::wait_for_startup() const
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  wait_for_startup_i();
}

ACE_INLINE
bool ReactorTask::is_shut_down() const
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  return state_ == STATE_SHUT_DOWN;
}

ACE_INLINE
ReactorInterceptor_rch ReactorTask::interceptor() const
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(lock_);
  wait_for_startup_i();
  return interceptor_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
