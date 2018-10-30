/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_ICE_TASK_H
#define OPENDDS_RTPS_ICE_TASK_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/Versioned_Namespace.h"
#include <ace/Time_Value.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

  class AgentImpl;

  struct Task {
    Task(AgentImpl * a_agent_impl) : m_agent_impl(a_agent_impl), m_in_queue(false) {}
    virtual ~Task() {};
    virtual void execute(ACE_Time_Value const & a_now) = 0;
    void enqueue(ACE_Time_Value const & release_time);
  private:
    friend class AgentImpl;
    AgentImpl * m_agent_impl;
    ACE_Time_Value m_release_time;
    bool m_in_queue;
  };

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_TASK_H */
