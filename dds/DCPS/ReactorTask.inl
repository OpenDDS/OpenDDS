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
  return reactor_;
}

ACE_INLINE
const ACE_Reactor* ReactorTask::get_reactor() const
{
  return reactor_;
}

ACE_INLINE
ACE_thread_t ReactorTask::get_reactor_owner() const
{
  return reactor_owner_;
}

ACE_INLINE
ACE_Proactor* ReactorTask::get_proactor()
{
  return proactor_;
}

ACE_INLINE
const ACE_Proactor* ReactorTask::get_proactor() const
{
  return proactor_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
