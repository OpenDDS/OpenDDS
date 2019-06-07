/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_ICE_H
#define OPENDDS_ICE_H

#include "dds/DCPS/dcps_export.h"
#include "ace/INET_Addr.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"

#include <sstream>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

template <typename T>
std::string stringify(T x)
{
  std::stringstream str;
  str << x;
  return str.str();
}

enum AgentType {
  FULL = 0x0,
  LITE = 0x1,
};

enum CandidateType {
  HOST = 0x0,
  SERVER_REFLEXIVE = 0x1,
  PEER_REFLEXIVE = 0x2,
  RELAYED = 0x3,
};

struct OpenDDS_Dcps_Export Candidate {
  ACE_INET_Addr address;
  // Transport - UDP or TCP
  std::string foundation;
  // Component ID
  uint32_t priority;
  CandidateType type;
  // Related Address and Port
  // Extensibility Parameters

  ACE_INET_Addr base;  // Not sent.

  bool operator==(const Candidate& other) const
  {
    return
      this->address == other.address &&
      this->foundation == other.foundation &&
      this->priority == other.priority &&
      this->type == other.type;
  }
};

bool candidates_sorted(const Candidate& x, const Candidate& y);
bool candidates_equal(const Candidate& x, const Candidate& y);

Candidate make_host_candidate(const ACE_INET_Addr& address);
Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address);
Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, uint32_t priority);
Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, uint32_t priority, size_t q);

struct OpenDDS_Dcps_Export AgentInfo {
  typedef std::vector<Candidate> CandidatesType;
  typedef CandidatesType::const_iterator const_iterator;

  CandidatesType candidates;
  AgentType type;
  // Connectivity-Check Pacing Value
  std::string username;
  std::string password;
  // Extensions

  const_iterator begin() const
  {
    return candidates.begin();
  }
  const_iterator end() const
  {
    return candidates.end();
  }
  bool operator==(const AgentInfo& other) const
  {
    return
      this->username == other.username &&
      this->password == other.password &&
      this->type == other.type &&
      this->candidates == other.candidates;
  }
  bool operator!=(const AgentInfo& other) const
  {
    return !(*this == other);
  }
};

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_ICE_H */
