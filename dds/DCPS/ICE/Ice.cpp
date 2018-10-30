/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Ice.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#if OPENDDS_SECURITY

bool
candidates_sorted(const Candidate& x, const Candidate& y)
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

Candidate
make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address)
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

Candidate
make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, uint32_t priority)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("P") + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate
make_peer_reflexive_candidate(const ACE_INET_Addr& address, uint32_t priority, size_t q)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("Q") + stringify(q) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  return candidate;
}

#endif /* OPENDDS_SECURITY */

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
