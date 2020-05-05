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

// TODO(jrw972): Implement RFC8421.

// TODO(jrw972): Implement NAT64 and DNS64 considerations.

// TODO(jrw972): For IPV6, prefer temporary addresses to permanent addresses.

// TODO(jrw972): If gathering one or more host candidates that
// correspond to an IPv6 address that was generated using a mechanism
// that prevents location tracking [RFC7721], host candidates that
// correspond to IPv6 addresses that do allow location tracking, are
// configured on the same interface, and are part of the same network
// prefix MUST NOT be gathered.  Similarly, when host candidates
// corresponding to an IPv6 address generated using a mechanism that
// prevents location tracking are gathered, then host candidates
// corresponding to IPv6 link-local addresses [RFC4291] MUST NOT be
// gathered.

ACE_UINT32 local_priority(const ACE_INET_Addr& addr)
{
  if (addr.get_type() == AF_INET6) {
    return 65535;
  }
  return 65534;
}

Candidate make_host_candidate(const ACE_INET_Addr& address)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation = std::string("H") + std::string(address.get_host_addr()) + "U";
  // See https://tools.ietf.org/html/rfc8445#section-5.1.2.1 for an explanation of the formula below.
  candidate.priority = (126 << 24) + (local_priority(address) << 8) + ((256 - 1) << 0); // No local preference, component 1.
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
  candidate.priority = (100 << 24) + (local_priority(address) << 8) + ((256 - 1) << 0); // No local preference, component 1.
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
