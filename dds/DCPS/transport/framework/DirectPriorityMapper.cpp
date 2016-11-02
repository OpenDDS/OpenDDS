/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "DCPS/debug.h"
#include "dds/DCPS/Service_Participant.h"

#include "DirectPriorityMapper.h"

#include <algorithm> // For std::min() and std::max()

#if !defined (__ACE_INLINE__)
#include "DirectPriorityMapper.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::DirectPriorityMapper::~DirectPriorityMapper()
{
}

short
OpenDDS::DCPS::DirectPriorityMapper::codepoint() const
{
  static const Priority dscp_min = 0;
  static const Priority dscp_max = 63;

  // We know that the DiffServ codepoints range from a low number to a
  // high number, with the high number being a higher priority - which
  // is the ordering that the TRANSPORT_PRIORIY value has.
  short value = std::min(dscp_max, std::max(dscp_min, this->priority()));

  if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DirectPriorityMapper:codepoint() - ")
               ACE_TEXT("mapped TRANSPORT_PRIORITY value %d ")
               ACE_TEXT("to codepoint %d.\n"),
               this->priority(),
               value));
  }

  return value;
}

short
OpenDDS::DCPS::DirectPriorityMapper::thread_priority() const
{
  static const int thread_min = TheServiceParticipant->priority_min();
  static const int thread_max = TheServiceParticipant->priority_max();
  static const int direction  = (thread_max < thread_min)? -1: 1;
  static const int range      = direction * (thread_max - thread_min);

  short value = thread_min + direction * this->priority();

  if (this->priority() < 0) {
    value = thread_min;
  }

  if (this->priority() > range) {
    value = thread_max;
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DirectPriorityMapper:thread_priority() - ")
               ACE_TEXT("mapped TRANSPORT_PRIORITY value %d ")
               ACE_TEXT("to thread priority %d.\n"),
               this->priority(),
               value));
  }

  return value;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
