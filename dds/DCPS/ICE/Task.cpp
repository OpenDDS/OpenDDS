/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Ice.h"

#include <iostream>
#include <sstream>

#include <openssl/rand.h>
#include <openssl/err.h>

#include "ace/Reactor.h"
#include "dds/DCPS/Service_Participant.h"
#include "Task.h"
#include "AgentImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  void Task::enqueue(ACE_Time_Value const & a_release_time) {
    m_release_time = a_release_time;
    m_agent_impl->enqueue(this);
  }

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
