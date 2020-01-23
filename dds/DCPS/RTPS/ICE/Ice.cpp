/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifdef OPENDDS_SECURITY

#include "Ice.h"

#include "AgentImpl.h"
#include "dds/DCPS/SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

bool candidates_sorted(const Candidate& x, const Candidate& y)
{
  if (x.address != y.address) {
    return x.address < y.address;
  }

  if (x.base != y.base) {
    return x.base < y.base;
  }

  return x.priority > y.priority;
}

bool candidates_equal(const Candidate& x, const Candidate& y)
{
  return x.address == y.address && x.base == y.base;
}

Candidate make_host_candidate(const ACE_INET_Addr& address)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("H") + std::string(address.get_host_addr()) + "U";
  // See https://tools.ietf.org/html/rfc8445#section-5.1.2.1 for an explanation of the formula below.
  candidate.priority = (126 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
  candidate.type = HOST;
  candidate.base = address;
  return candidate;
}

Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("S") + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
  // See https://tools.ietf.org/html/rfc8445#section-5.1.2.1 for an explanation of the formula below.
  candidate.priority = (100 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
  candidate.type = SERVER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, ACE_UINT32 priority)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("P") + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, ACE_UINT32 priority, size_t q)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("Q") + OpenDDS::DCPS::to_dds_string(q) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  return candidate;
}

Agent* Agent::instance()
{
  return ACE_Singleton<AgentImpl, ACE_Thread_Mutex>::instance();
}


} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_SECURITY */
