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
  agent_impl_->enqueue(a_release_time, rchandle_from(this));
}

#endif /* OPENDDS_SECURITY */

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
