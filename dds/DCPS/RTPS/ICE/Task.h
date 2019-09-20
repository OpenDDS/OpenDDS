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
#include "dds/DCPS/TimeTypes.h"
#include <ace/Time_Value.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

class AgentImpl;

struct Task {
  Task(AgentImpl* a_agent_impl) : agent_impl_(a_agent_impl), in_queue_(false) {}
  virtual ~Task() {};
  virtual void execute(const DCPS::MonotonicTimePoint& a_now) = 0;
  void enqueue(const DCPS::MonotonicTimePoint& release_time);
private:
  friend class AgentImpl;
  AgentImpl* agent_impl_;
  DCPS::MonotonicTimePoint release_time_;
  bool in_queue_;
};

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_ICE_TASK_H */
