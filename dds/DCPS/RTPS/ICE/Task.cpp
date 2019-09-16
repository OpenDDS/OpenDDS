/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Task.h"

#include "AgentImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#ifdef OPENDDS_SECURITY

void Task::enqueue(const DCPS::MonotonicTimePoint& a_release_time)
{
  release_time_ = a_release_time;
  agent_impl_->enqueue(this);
}

#endif /* OPENDDS_SECURITY */

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
