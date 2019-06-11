/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Ice.h"

#include "AgentImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#if OPENDDS_SECURITY

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
  candidate.foundation += "H" + std::string(address.get_host_addr()) + "U";
  candidate.priority = (126 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
  candidate.type = HOST;
  candidate.base = address;
  return candidate;
}

Candidate make_server_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation += "S" + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
  candidate.priority = (100 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
  candidate.type = SERVER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& base, const ACE_INET_Addr& server_address, ACE_UINT32 priority)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation += "P" + std::string(base.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  candidate.base = base;
  return candidate;
}

Candidate make_peer_reflexive_candidate(const ACE_INET_Addr& address, ACE_UINT32 priority, size_t q)
{
  Candidate candidate;
  candidate.address = address;
  candidate.foundation += "Q" + stringify(q) + "U";
  candidate.priority = priority;
  candidate.type = PEER_REFLEXIVE;
  return candidate;
}

// Candidate make_relayed_candidate(const ACE_INET_Addr& address, const ACE_INET_Addr& server_address) {
//   Candidate candidate;
//   candidate.address = address;
//   candidate.foundation = "R" + std::string(address.get_host_addr()) + "_" + std::string(server_address.get_host_addr()) + "U";
//   candidate.priority = (0 << 24) + (65535 << 8) + ((256 - 1) << 0); // No local preference, component 1.
//   candidate.type = RELAYED;
//   candidate.base = address;
//   return candidate;
// }

Agent* Agent::instance()
{
  return ACE_Singleton<AgentImpl, ACE_SYNCH_MUTEX>::instance();
}

std::ostream& operator<<(std::ostream& stream, const ACE_INET_Addr& address)
{
  stream << address.get_host_addr() << ':' << address.get_port_number();
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const STUN::TransactionId& tid)
{
  for (size_t idx = 0; idx != 12; ++idx) {
    stream << int(tid.data[idx]);
  }

  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ICE::Candidate& candidate)
{
  stream << "address:    " << candidate.address << '\n'
         << "foundation: " << candidate.foundation << '\n'
         << "priority:   " << candidate.priority << '\n'
         << "type:       " << candidate.type << '\n'
         << "base:       " << candidate.base << '\n';
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ICE::AgentInfo& agent_info)
{
  stream << "type:     " << agent_info.type     << '\n'
         << "username: " << agent_info.username << '\n'
         << "password: " << agent_info.password << '\n';

  for (AgentInfo::const_iterator pos = agent_info.begin(), limit = agent_info.end(); pos != limit; ++pos) {
    stream << *pos;
  }

  return stream;
}

#endif /* OPENDDS_SECURITY */

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
